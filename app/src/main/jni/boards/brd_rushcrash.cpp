/* Rush and Crash / Speed Rumbler */

#include "m1snd.h"

static void RNC_Init(long srate);
static void RNC_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct YM2203interface ym2203_interface =
{
	2,                      /* 2 chips */
	4000000,        /* 4.0 MHz (? hand tuned to match the real board) */
	{ YM2203_VOL(40,20), YM2203_VOL(40,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MEMORY_READ_START( sound_readmem )
	{ 0xe000, 0xe000, latch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0x0000, 0x7fff, MWA_ROM },
MEMORY_END

M1_BOARD_START( rushcrash )
	MDRV_NAME("Rush and Crash")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( RNC_Init )
	MDRV_SEND( RNC_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static void s86_timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	// set up for next time
	timer_set(1.0/240.0, 0, s86_timer);
}

static void RNC_Init(long srate)
{
	timer_set(1.0/240.0, 0, s86_timer);
}

static void RNC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

