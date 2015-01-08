/* Atari System 2 */
/* NOTE: this and Gauntlet both work off a scanline interrupt rather than 2151
   timer IRQs.  dunno why... */

#include "m1snd.h"

#define ATARI_CLOCK_14MHz        14318180
#define ATARI_CLOCK_20MHz        20000000
#define M6502_CLOCK (ATARI_CLOCK_14MHz/8)

#define TIMER_RATE (0.004005)	// taken from system 1

static void PB_Init(long srate);
static void PB_SendCmd(int cmda, int cmdb);

static unsigned int pb_readmem(unsigned int address);
static void pb_writemem(unsigned int address, unsigned int data);

static int cmd_latch, status = 0;
static int ym2151_last_adr;
static int tms5220_data, tms5220_data_strobe;

static M16502T mmrw = 
{
	pb_readmem,
	pb_readmem,
	pb_writemem,
};

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
};


static struct POKEYinterface pokey_interface =
{
	2,
	ATARI_CLOCK_14MHz/8,
	{ MIXER(60,MIXER_PAN_LEFT), MIXER(60,MIXER_PAN_RIGHT) },
};


static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_20MHz/4/4/2,
	100,
	0
};

static unsigned int pb_readmem(unsigned int address)
{
	if (address < 0x1800)
	{	
		return workram[address];
	}

	if (address >= 0x1800 && address <= 0x180f)
	{
		return pokey1_r(address & 0xf);
	}

	if (address >= 0x1810 && address <= 0x1813)
	{
		return 0xff;	// LETA analog inputs
	}

	if (address >= 0x1830 && address <= 0x183f)
	{
		return pokey2_r(address & 0xf);
	}

	if (address == 0x1840)
	{
		if (tms5220_ready_r()) status &= ~0x04;
			else status |= 0x04;

		return status;
	}

	if (address >= 0x1850 && address <= 0x1851)
	{
		return YM2151ReadStatus(0);
	}

	if (address == 0x1860)
	{
//		printf("reading cmd %d\n", cmd_latch);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		status &= ~0x01;
		return cmd_latch;
	}
	
	if (address >= 0x4000)
	{
		return prgrom[address-0x4000];
	}

//	printf("Unmapped read from %x\n", address);
	return 0;
}

static void pb_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x1800)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x1800 && address <= 0x180f)
	{
		pokey1_w(address & 0xf, data);
		return;
	}

	if (address >= 0x1830 && address <= 0x183f)
	{
		pokey2_w(address & 0xf, data);
		return;
	}

	if (address == 0x1850)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x1851)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address == 0x1878)
	{
		return;
	}

	if (address >= 0x1870 && address <= 0x187e)
	{
		address -= 0x1870;

		switch (address)
		{
			case 0:
//				printf("5220: data latch to %x\n", data);
				tms5220_data = data;
				break;

			case 2:
			case 3:
				if (!(address & 1) && tms5220_data_strobe)
				{
//					printf("write %x to 5220\n", tms5220_data);
					tms5220_data_w(0, tms5220_data);
				}

				tms5220_data_strobe = address&0x1;
				break;
			case 0xc:
				data = 12 | ((data >> 5) & 1);
				tms5220_set_frequency(ATARI_CLOCK_20MHz/4 / (16 - data) / 2);
				break;
		}

		return;	// various writes we don't care about
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void timer(int ref)
{
	cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);
	cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void PB_Init(long srate)
{
	tms5220_data_strobe = 1;

	memset(&workram[0x1000], 0xff, 0x800);

	timer_set(TIMER_RATE, 0, timer);
}

static void PB_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	status |= 0x01;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( ataris2 )
	MDRV_NAME("Atari System 2")
	MDRV_HWDESC("6502, YM2151, TMS5220, POKEY(x2)")
	MDRV_INIT( PB_Init )
	MDRV_SEND( PB_SendCmd )
	MDRV_DELAYS( 500, 15 )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&mmrw)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(POKEY, &pokey_interface)
	MDRV_SOUND_ADD(TMS5220, &tms5220_interface)
M1_BOARD_END
