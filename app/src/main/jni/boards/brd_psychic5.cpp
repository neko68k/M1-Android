/* Jaleco Psychic 5 */

#include "m1snd.h"

static int cmd_latch;

static READ_HANDLER( latch_r )
{
//	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_WRITE_START( sound_readport )
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
PORT_END

static void irqhandler(int irq)
{
	cpu_set_irq_line(0,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,		/* 2 chips   */
	6000000/4,    	/* 1.5 MHz */
	{ YM2203_VOL(50,15), YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static void P5_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
//	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( psychic5 )
	MDRV_NAME("Psychic 5")
	MDRV_HWDESC("Z80, YM2203(x2)")
	MDRV_SEND( P5_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

