/* Atari System 1 */

#include "m1snd.h"

#define ATARI_CLOCK_14MHz        14318180

#define M6502_CLOCK (ATARI_CLOCK_14MHz/8)

static void MM_Init(long srate);
static void MM_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);

static unsigned int mm_readmem(unsigned int address);
static void mm_writemem(unsigned int address, unsigned int data);

static int cmd_latch, status = 0;
static int ym2151_last_adr;

static UINT8 m6522_ddra, m6522_ddrb;
static UINT8 m6522_dra, m6522_drb;
static UINT8 m6522_regs[16];

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};


static struct POKEYinterface pokey_interface =
{
	1,
	ATARI_CLOCK_14MHz/8,
	{ 40 },
};


static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_14MHz/2/11,
	100,
	0
};


static M16502T mmrw = 
{
	mm_readmem,
	mm_readmem,
	mm_writemem,
};

/*
 *	All communication to the 5220 goes through an SY6522A, which is an overpowered chip
 *	for the job.  Here is a listing of the I/O addresses:
 *
 *		$00	DRB		Data register B
 *		$01	DRA		Data register A
 *		$02	DDRB	Data direction register B (0=input, 1=output)
 *		$03	DDRA	Data direction register A (0=input, 1=output)
 *		$04	T1CL	T1 low counter
 *		$05	T1CH	T1 high counter
 *		$06	T1LL	T1 low latches
 *		$07	T1LH	T1 high latches
 *		$08	T2CL	T2 low counter
 *		$09	T2CH	T2 high counter
 *		$0A	SR		Shift register
 *		$0B	ACR		Auxiliary control register
 *		$0C	PCR		Peripheral control register
 *		$0D	IFR		Interrupt flag register
 *		$0E	IER		Interrupt enable register
 *		$0F	NHDRA	No handshake DRA
 *
 *	Fortunately, only addresses $00,$01,$0B,$0C, and $0F are accessed in the code, and
 *	$0B and $0C are merely set up once.
 *
 *	The ports are hooked in like follows:
 *
 *	Port A, D0-D7 = TMS5220 data lines (i/o)
 *
 *	Port B, D0 = 	Write strobe (out)
 *	        D1 = 	Read strobe (out)
 *	        D2 = 	Ready signal (in)
 *	        D3 = 	Interrupt signal (in)
 *	        D4 = 	LED (out)
 *	        D5 = 	??? (out)
 */

static READ_HANDLER( m6522_r )
{
	switch (offset)
	{
		case 0x00:	/* DRB */
			return (m6522_drb & m6522_ddrb) | (!tms5220_ready_r() << 2) | (!tms5220_int_r() << 3);

		case 0x01:	/* DRA */
		case 0x0f:	/* NHDRA */
			return (m6522_dra & m6522_ddra);

		case 0x02:	/* DDRB */
			return m6522_ddrb;

		case 0x03:	/* DDRA */
			return m6522_ddra;

		default:
			return m6522_regs[offset & 15];
	}
}


WRITE_HANDLER( m6522_w )
{
	int old;

	switch (offset)
	{
		case 0x00:	/* DRB */
			old = m6522_drb;
			m6522_drb = (m6522_drb & ~m6522_ddrb) | (data & m6522_ddrb);
			if (!(old & 1) && (m6522_drb & 1))
				tms5220_data_w(0, m6522_dra);
			if (!(old & 2) && (m6522_drb & 2))
				m6522_dra = (m6522_dra & m6522_ddra) | (tms5220_status_r(0) & ~m6522_ddra);

			/* bit 4 is connected to an up-counter, clocked by SYCLKB */
			data = 5 | ((data >> 3) & 2);
			tms5220_set_frequency(ATARI_CLOCK_14MHz/2 / (16 - data));
			break;

		case 0x01:	/* DRA */
		case 0x0f:	/* NHDRA */
			m6522_dra = (m6522_dra & ~m6522_ddra) | (data & m6522_ddra);
			break;

		case 0x02:	/* DDRB */
			m6522_ddrb = data;
			break;

		case 0x03:	/* DDRA */
			m6522_ddra = data;
			break;

		default:
			m6522_regs[offset & 15] = data;
			break;
	}
}

static unsigned int mm_readmem(unsigned int address)
{
	if (address < 0x1000)
	{	
		return workram[address];
	}

	if (address >= 0x1000 && address <= 0x100f)
	{
		return m6522_r(address&0xf);
	}

	if (address >= 0x1800 && address <= 0x1801)
	{
		return YM2151ReadStatus(0);
	}

	if (address == 0x1810)
	{
//		printf("reading %x from latch\n", cmd_latch);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		status &= ~0x08;
		return cmd_latch;
	}
	
	if (address == 0x1820)
	{
		return status;
	}	

	if (address >= 0x1870 && address <= 0x187f)
	{
		return pokey1_r(address & 0xf);	// pokey 
	}

	if (address >= 0x4000)
	{
		return prgrom[address-0x4000];
	}

//	printf("Unmapped read from %x\n", address);

	return 0;
}

static void mm_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x1000)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x1000 && address <= 0x100f)
	{
		m6522_w(address&0xf, data);
		return;
	}

	if (address == 0x1800)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x1801)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x1810 && address <= 0x1827)
	{
		return;	// command reply to the main CPU, LEDs, coin counters
	}

	if (address >= 0x1870 && address <= 0x187f)
	{
		pokey1_w(address & 0xf, data);
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}
 
static void YM2151_IRQ(int irq)
{
	if (irq)
		cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);
}

static void MM_Init(long srate)
{
	/* reset the 6522 controller */
	m6522_ddra = m6522_ddrb = 0xff;
	m6522_dra = m6522_drb = 0xff;
	memset(m6522_regs, 0xff, sizeof(m6522_regs));
}

static void MM_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	status |= 0x08;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( ataris1 )
	MDRV_NAME("Atari System 1")
	MDRV_HWDESC("6502, YM2151, TMS5220, POKEY")
	MDRV_INIT( MM_Init )
	MDRV_SEND( MM_SendCmd )
	MDRV_DELAYS( 1200, 15 )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&mmrw)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(POKEY, &pokey_interface)
	MDRV_SOUND_ADD(TMS5220, &tms5220_interface)
M1_BOARD_END
