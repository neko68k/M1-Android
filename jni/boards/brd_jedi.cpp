/* Atari "Return of the Jedi" */

#include "m1snd.h"

#define SOUND_CPU_OSC		12096000

#define TIMER_LINE	M6502_IRQ_LINE
#define TIMER_RATE 	(1.0 / (60.0 * 8.0))	// toggle every 32 scanlines, there are 256 scanlines total

static void ROTJ_Init(long srate);
static void ROTJ_SendCmd(int cmda, int cmdb);

static unsigned int jedi_readmem(unsigned int address);
static void jedi_writemem(unsigned int address, unsigned int data);

static UINT8 sound_latch;
static UINT8 sound_ack_latch;
static UINT8 sound_comm_stat;
static UINT8 speech_write_buffer;
static UINT8 speech_strobe_state;

static struct POKEYinterface pokey_interface =
{
	4,			/* 4 chips */
	SOUND_CPU_OSC/2/4,
	{ 20, 20, 20, 20 },	/* volume */
	/* The 8 pot handlers */
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0, 0 },
};


static struct TMS5220interface tms5220_interface =
{
	SOUND_CPU_OSC/2/9,     /* clock speed (80*samplerate) */
	50,         /* volume */
	0           /* IRQ handler */
};

static M16502T swrw = 
{
	jedi_readmem,
	jedi_readmem,
	jedi_writemem,
};

M1_BOARD_START( jedi )
	MDRV_NAME( "Return of the Jedi" )
	MDRV_HWDESC( "6502, POKEY(x4), TMS5220" )
	MDRV_DELAYS( 1200, 50 )
	MDRV_INIT( ROTJ_Init )
	MDRV_SEND( ROTJ_SendCmd )

	MDRV_CPU_ADD(M6502, SOUND_CPU_OSC/2/4)
	MDRV_CPUMEMHAND(&swrw)

	MDRV_SOUND_ADD(POKEY, &pokey_interface)
	MDRV_SOUND_ADD(TMS5220, &tms5220_interface)
M1_BOARD_END

static unsigned int jedi_readmem(unsigned int address)
{
	if (address <= 0x7ff)
	{	
		return workram[address];
	}

	if (address >= 0x800 && address <= 0x80f)
	{
		return pokey1_r(address&0xf);
	}
	if (address >= 0x810 && address <= 0x81f)
	{
		return pokey2_r(address&0xf);
	}
	if (address >= 0x820 && address <= 0x82f)
	{
		return pokey3_r(address&0xf);
	}
	if (address >= 0x830 && address <= 0x83f)
	{
		return pokey4_r(address&0xf);
	}

	if (address == 0x1800)
	{
//		printf("read %x from latch\n", sound_latch);
	    sound_comm_stat &= ~0x80;
	    return sound_latch;
	}

	if (address == 0x1c00)
	{
	    return (!tms5220_ready_r()) << 7;
	}

	if (address == 0x1c01)
	{
	    return sound_comm_stat;
	}

	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read from %x\n", address);
	return 0;
}

static void jedi_writemem(unsigned int address, unsigned int data)
{
	if (address <= 0x7ff)
	{
		workram[address] = data;
		return;
	}

	if (address >= 0x800 && address <= 0x80f)
	{
		pokey1_w(address&0xf, data);
		return;
	}
	if (address >= 0x810 && address <= 0x81f)
	{
		pokey2_w(address&0xf, data);
		return;
	}
	if (address >= 0x820 && address <= 0x82f)
	{
		pokey3_w(address&0xf, data);
		return;
	}
	if (address >= 0x830 && address <= 0x83f)
	{
		pokey4_w(address&0xf, data);
		return;
	}

	if (address == 0x1000)
	{
		cpu_set_irq_line(0, TIMER_LINE, CLEAR_LINE);
	}

	if (address >= 0x1100 && address <= 0x11ff)
	{
		speech_write_buffer = data;
	}

	if (address >= 0x1200 && address <= 0x13ff)
	{
		int state = (~(address-0x1200) >> 8) & 1;
	
		if ((state ^ speech_strobe_state) && state)
			tms5220_data_w(0, speech_write_buffer);
		speech_strobe_state = state;
	}

	if (address == 0x1400)
	{
	    sound_comm_stat |= 0x40;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static int tstat = 0;

static void timer(int ref)
{
	tstat ^= 1;
	if (tstat)
		cpu_set_irq_line(0, TIMER_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, TIMER_LINE, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void ROTJ_Init(long srate)
{
	sound_latch = 0;
	sound_ack_latch = 0;
	sound_comm_stat = 0;
	speech_write_buffer = 0;
	speech_strobe_state = 0;

	timer_set(TIMER_RATE, 0, timer);

	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0xff);
	m1snd_addToCmdQueueRaw(1);
	m1snd_addToCmdQueueRaw(0);

	m1snd_setCmdPrefix(0);
}

static void ROTJ_SendCmd(int cmda, int cmdb)
{
	sound_latch = cmda;
	sound_comm_stat |= 0x80;
}
