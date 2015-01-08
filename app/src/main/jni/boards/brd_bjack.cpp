/* Bombjack */

#include "m1snd.h"

#define BJCK_Z80_CLOCK (3072000)

static void BJCK_Init(long srate);
static void BJCK_SendCmd(int cmda, int cmdb);

#define TIMER_RATE (1.0/60.0)	// 240 times per second

static int cmd_latch;

static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 13, 13, 13 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static READ_HANDLER( latch_r )
{
	int latch = cmd_latch&0xff;

	cmd_latch = 0;

	return latch;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( bombjack_sound_readport )
PORT_END

static PORT_WRITE_START( bombjack_sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x11, 0x11, AY8910_write_port_1_w },
	{ 0x80, 0x80, AY8910_control_port_2_w },
	{ 0x81, 0x81, AY8910_write_port_2_w },
PORT_END

M1_BOARD_START( bjack )
	MDRV_NAME( "Bomb Jack" )
	MDRV_HWDESC( "Z80, AY-3-8910(x3)" )
	MDRV_DELAYS( 60, 60 )
	MDRV_INIT( BJCK_Init )
	MDRV_SEND( BJCK_SendCmd )

	MDRV_CPU_ADD(Z80C, BJCK_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_PORTS(bombjack_sound_readport, bombjack_sound_writeport)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END

static void timer(int ref)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void BJCK_Init(long srate)
{
	timer_set(TIMER_RATE, 0, timer);
}

static void BJCK_SendCmd(int cmda, int cmdb)
{
	if ((cmda < 16) && (cmda != 1)) cmda = 1;

	cmd_latch = cmda;
}
