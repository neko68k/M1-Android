/* Gauntlet / Gauntlet II / Vindicators - similar to System 1/2 */
/* NOTE: this and System 2 both work off a scanline interrupt rather than 2151
   timer IRQs.  dunno why... */

#include "m1snd.h"

#define ATARI_CLOCK_14MHz        14318180

#define M6502_CLOCK (ATARI_CLOCK_14MHz/8)

#define TIMER_RATE (0.004005)	// taken from system 1

static void GNT_Init(long srate);
static void GNT_SendCmd(int cmda, int cmdb);

static unsigned int gnt_readmem(unsigned int address);
static void gnt_writemem(unsigned int address, unsigned int data);

static int cmd_latch, status = 0;
static int ym2151_last_adr;

static UINT8 speech_val;
static UINT8 last_speech_write;
static UINT8 speech_squeak;

static M16502T mmrw = 
{
	gnt_readmem,
	gnt_readmem,
	gnt_writemem,
};

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ YM3012_VOL(48,MIXER_PAN_LEFT,48,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct POKEYinterface pokey_interface =
{
	1,
	ATARI_CLOCK_14MHz/8,
	{ 32 },
};


static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_14MHz/2/11,	/* potentially ATARI_CLOCK_14MHz/2/9 as well */
	80,
	0
};

static unsigned int gnt_readmem(unsigned int address)
{
	if (address < 0x1000)
	{	
		return workram[address];
	}

//	if (address < 0x4000)
//		printf("read at %x\n", address);

	if (address == 0x1010)
	{
//		printf("reading cmd %d\n", cmd_latch);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		status &= ~0x80;
		return cmd_latch;
	}

	if (address >= 0x1020 && address <= 0x102f)
	{	
		return 0;	// coin inputs in lower 4 bits
	}

	if (address >= 0x1030 && address <= 0x103f)
	{
		if (tms5220_ready_r()) status &= ~0x20;
			else status |= 0x20;

		return status;
	}

	if (address >= 0x1800 && address <= 0x180f)
	{
		return pokey1_r(address & 0xf);
	}

	if (address == 0x1811)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x1830 && address <= 0x183f)
	{
//		printf("IRQ ack (R)\n");
		cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);
		return 0;
	}

	if (address >= 0x4000)
	{
		return prgrom[address-0x4000];
	}

//	printf("Unmapped read from %x\n", address);
	return 0;
}

static void gnt_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x1000)
	{	
		workram[address] = data;
		return;
	}

	if (address == 0x1000)
	{
		return; // response write
	}

	if (address >= 0x1020 && address <= 0x102f)
	{
		return;
	}

	if (address >= 0x1030 && address <= 0x103f)
	{
		// 1030 = YM2151 reset
		// 1031 = TMS5220 write
		// 1032 = TMS5220 reset
		// 1033 = TMS5220 "squeak"

		if (address == 0x1031)
		{
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w(0, speech_val);
			last_speech_write = data;
			return;
		}

		if (address == 0x1032)
		{
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_reset();
			return;
		}

		if (address == 0x1033)
		{
			data = 5 | ((data >> 6) & 2);
			tms5220_set_frequency(ATARI_CLOCK_14MHz/2 / (16 - data));
			return;
		}

//		printf("write to sound_ctl_w\n");
		return;
	}

	if (address >= 0x1800 && address <= 0x180f)
	{
		pokey1_w(address & 0xf, data);
		return;
	}

	if (address == 0x1810)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x1811)
	{
  //		printf("write %x to 2151 register %x\n", data, ym2151_last_adr);
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x1820 && address <= 0x182f)
	{
		speech_val = data;
		return;	// 5220 data latch
	}

	if (address >= 0x1830 && address <= 0x183f)
	{
//		printf("IRQ ack (W)\n");
		cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);
		return; // IRQ ack
	}

//	printf("Unmapped write %x to %x\n", data, address);
}
 
static void timer(int ref)
{
	cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void GNT_Init(long srate)
{
	last_speech_write = 0x80;
	speech_squeak = 0;

	memset(&workram[0x1000], 0xff, 0x800);

	status = 0x80;	// MUST be this on boot or the program will immediately die

	timer_set(TIMER_RATE, 0, timer);
}

static void GNT_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	status |= 0x80;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( gauntlet )
	MDRV_NAME("Gauntlet")
	MDRV_HWDESC("6502, YM2151, TMS5220, POKEY")
	MDRV_INIT( GNT_Init )
	MDRV_SEND( GNT_SendCmd )
	MDRV_DELAYS( 2000, 15 )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&mmrw)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(POKEY, &pokey_interface)
	MDRV_SOUND_ADD(TMS5220, &tms5220_interface)
M1_BOARD_END
