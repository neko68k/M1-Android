/* Capcom 1942 */

#include "m1snd.h"

#define C1942_Z80_CLOCK (3000000)

static void C1942_Init(long srate);
static void C1942_SendCmd(int cmda, int cmdb);

#define TIMER_RATE (1.0/240.0)	// 240 times per second

static int cmd_latch;

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 15, 15 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static READ_HANDLER( latch_r )
{
	return cmd_latch&0xff;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0xc000, 0xc000, AY8910_control_port_1_w },
	{ 0xc001, 0xc001, AY8910_write_port_1_w },
MEMORY_END

M1_BOARD_START( 1942 )
	MDRV_NAME( "Capcom 1942" )
	MDRV_HWDESC( "Z80, AY-3-8910(x3)" )
	MDRV_DELAYS( 200, 200 )
	MDRV_INIT( C1942_Init )
	MDRV_SEND( C1942_SendCmd )

	MDRV_CPU_ADD(Z80C, C1942_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END

static void timer(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void C1942_Init(long srate)
{
	timer_set(TIMER_RATE, 0, timer);
}

static void C1942_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
