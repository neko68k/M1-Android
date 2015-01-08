// DECO/Sega pinball sound board: M6809 + BSMT2000

#include "m1snd.h"

#define M6809_CLOCK (3579580/2)
#define TIMER_RATE (1.0/(489.0))	// FIRQ at 489 Hz measured on real PCB

static void TA_Init(long srate);
static void TA_SendCmd(int cmda, int cmdb);

static unsigned int TA_Read(unsigned int address);
static void TA_Write(unsigned int address, unsigned int data);

static M16809T tarwmem =
{
	TA_Read,
	TA_Write,
};

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ 24000000 },
	{ 11 },
	{ RGN_SAMP1 },
	{ 100 }
};

static struct BSMT2000interface bsmt2000_interface_12 =
{
	1,
	{ 24000000 },
	{ 12 },
	{ RGN_SAMP1 },
	{ 100 }
};

static struct 
{
	int bufFull;
	int soundCmd;
	int bsmtData;
} locals;

static unsigned int TA_Read(unsigned int address)
{
	if (address <= 0x1fff) return workram[address];
	if (address >= 0x2002 && address <= 0x2003)
	{
		locals.bufFull = FALSE;
		return locals.soundCmd;
	}
	if (address >= 0x2006 && address <= 0x2007)
	{
		return 0x80;	// BSMT is always ready
	}

	if (address >= 0x4000) return prgrom[address];

	return 0;
}

static void TA_Write(unsigned int address, unsigned int data)
{					
	if (address <= 0x1fff) 
	{
		workram[address] = data;
		return;
	}

	if (address >= 0x2000 && address <= 0x2001)
	{
		return;
	}

	if (address == 0x6000)
	{
		locals.bsmtData = data;
		return;
	}

	if (address >= 0xa000 && address <= 0xa0ff)
	{
		int reg = (address & 0xff) ^ 0xff;

//		printf("BSMT write to %02X (V%x R%X) = %02X%02X\n", reg, reg % 11, reg / 11, locals.bsmtData, data);

		BSMT2000_data_0_w(reg, ((locals.bsmtData<<8)|data), 0);

		cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
		return;
	}
}

static void gx_timer(int refcon)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);

	timer_set(TIMER_RATE, 0, gx_timer);
}

static void TA_Init(long srate)
{
	timer_set(TIMER_RATE, 0, gx_timer);

	m1snd_addToCmdQueue(0);

	locals.soundCmd = 2;
}

static void TA_SendCmd(int cmda, int cmdb)
{
	locals.soundCmd = cmda;
}

M1_BOARD_START( tatass )
	MDRV_NAME( "Tattoo Assassins" )
	MDRV_HWDESC( "M6809, BSMT2000 (11 voice)" )
	MDRV_DELAYS( 400, 20 )
	MDRV_INIT( TA_Init )
	MDRV_SEND( TA_SendCmd )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&tarwmem)

	MDRV_SOUND_ADD(BSMT2000, &bsmt2000_interface)
M1_BOARD_END

M1_BOARD_START( whitestar )
	MDRV_NAME( "Whitestar" )
	MDRV_HWDESC( "M6809, BSMT2000 (12 voice)" )
	MDRV_DELAYS( 400, 20 )
	MDRV_INIT( TA_Init )
	MDRV_SEND( TA_SendCmd )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&tarwmem)

	MDRV_SOUND_ADD(BSMT2000, &bsmt2000_interface_12)
M1_BOARD_END
