/* Tecmo16: Final Star Force and Ganbare Ginkun */

#include "m1snd.h"

static void T16_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct YM2151interface ym2151_interface =
{
	1,
	8000000/2,	/* 4MHz */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ YM_IRQHandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 7575 },		/* 7575Hz playback */
	{ REGION_SOUND1 },
	{ 40 }
};

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xfbff, MRA_RAM },	/* Sound RAM */
	{ 0xfc00, 0xfc00, OKIM6295_status_0_r },
	{ 0xfc05, 0xfc05, YM2151_status_port_0_r },
	{ 0xfc08, 0xfc08, latch_r },
	{ 0xfc0c, 0xfc0c, MRA_NOP },
	{ 0xfffe, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xfbff, MWA_RAM },	/* Sound RAM */
	{ 0xfc00, 0xfc00, OKIM6295_data_0_w },
	{ 0xfc04, 0xfc04, YM2151_register_port_0_w },
	{ 0xfc05, 0xfc05, YM2151_data_port_0_w },
	{ 0xfc0c, 0xfc0c, MWA_NOP },
	{ 0xfffe, 0xffff, MWA_RAM },
MEMORY_END

M1_BOARD_START( tecmo16 )
	MDRV_NAME("Tecmo16")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000/2)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static void YM_IRQHandler(int irq)
{
//	printf("YM IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);

}

static void T16_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
