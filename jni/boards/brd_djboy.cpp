/* DJ Boy and Heavy Unit by Kaneko */

#include "m1snd.h"

static void DJB_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static void YM_IRQHandler(int irq);

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct YM2203interface ym2203_interface =
{
	1,	/* 1 chips */
	3000000, //3579545,
	{ YM2203_VOL(90,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQHandler }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 7000, 7000 },
	{ RGN_SAMP1, RGN_SAMP1 },
	{ 100, 100 }
};

static WRITE_HANDLER( z80_bank_w )
{
	int soundbank = data * 0x4000;
	data8_t *Z80 = (data8_t *)memory_region(REGION_CPU1);

	cpu_setbank(1, &Z80[soundbank]);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x02, 0x02, YM2203_status_port_0_r },
	{ 0x04, 0x04, latch_r },
	{ 0x06, 0x06, OKIM6295_status_0_r },
	{ 0x07, 0x07, OKIM6295_status_1_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, z80_bank_w },
	{ 0x02, 0x02, YM2203_control_port_0_w },
	{ 0x03, 0x03, YM2203_write_port_0_w },
	{ 0x06, 0x06, OKIM6295_data_0_w },
	{ 0x07, 0x07, OKIM6295_data_1_w },
PORT_END

M1_BOARD_START( djboy )
	MDRV_NAME("DJ Boy")
	MDRV_HWDESC("Z80, YM2203, MSM-6295(x2)")
	MDRV_DELAYS(200, 50)
	MDRV_SEND( DJB_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readport, writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static MEMORY_READ_START( hureadmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( huwritemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( hureadport )
	{ 0x02, 0x02, YM2203_status_port_0_r },
	{ 0x04, 0x04, latch_r },
PORT_END

static PORT_WRITE_START( huwriteport )
	{ 0x00, 0x00, z80_bank_w },
	{ 0x02, 0x02, YM2203_control_port_0_w },
	{ 0x03, 0x03, YM2203_write_port_0_w },
PORT_END

M1_BOARD_START( hvyunit )
	MDRV_NAME("Heavy Unit")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_DELAYS(200, 50)
	MDRV_SEND( DJB_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(hureadmem, huwritemem)
	MDRV_CPU_PORTS(hureadport, huwriteport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static void YM_IRQHandler(int irq)	// irqs are ignored by most games
{
//	printf("YM IRQ: %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void DJB_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

