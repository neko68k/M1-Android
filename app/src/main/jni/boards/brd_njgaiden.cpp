/* Tecmo Ninja Gaiden and friends */
/* Z80 + 2x YM2203 + MSM-6295 */

#include "m1snd.h"

static int cmd_latch;

static void Ninja_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz.  Audio section crystal on Strato Fighter is 4.000 MHz */
	{ YM2203_VOL(60,15), YM2203_VOL(60,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  	/* 1 chip */
	{ 7576 },		/* 7576Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 20 }
};

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, OKIM6295_status_0_r },
	{ 0xfc00, 0xfc00, MRA_NOP },	/* ?? */
	{ 0xfc20, 0xfc20, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, OKIM6295_data_0_w },
	{ 0xf810, 0xf810, YM2203_control_port_0_w },
	{ 0xf811, 0xf811, YM2203_write_port_0_w },
	{ 0xf820, 0xf820, YM2203_control_port_1_w },
	{ 0xf821, 0xf821, YM2203_write_port_1_w },
	{ 0xfc00, 0xfc00, MWA_NOP },	/* ?? */
MEMORY_END

M1_BOARD_START( ninjag )
	MDRV_NAME("Ninja Gaiden")
	MDRV_HWDESC("Z80, YM2203(x2), MSM-6295")
	MDRV_SEND( Ninja_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END
