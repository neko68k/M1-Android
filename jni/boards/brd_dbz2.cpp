/* DBZ2 YM2151 board */

#include "m1snd.h"

#define Z80_CLOCK (4000000)
#define YM_CLOCK (4000000)

static void DBZ2_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);

static int cmd_latch;

static struct YM2151interface ym2151_interface =
{
	1,
	YM_CLOCK,
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static struct OKIM6295interface okim6295_interface =
{
	1,  /* 1 chip */
	{ 1056000/132 },	/* confirmed */
	{ RGN_SAMP1 },
	{ 80 }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( dbz2sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc001, YM2151_status_port_0_r },
	{ 0xd000, 0xd002, OKIM6295_status_0_r },
	{ 0xe000, 0xe001, latch_r },
MEMORY_END

static MEMORY_WRITE_START( dbz2sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd001, OKIM6295_data_0_w },
MEMORY_END

static PORT_READ_START( dbz2sound_readport )
PORT_END

static PORT_WRITE_START( dbz2sound_writeport )
	{ 0x00, 0x00, IOWP_NOP },
PORT_END

M1_BOARD_START( dbz2 )
	MDRV_NAME("Banpresto DBZ")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_DELAYS( 120, 30 )
	MDRV_SEND( DBZ2_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(dbz2sound_readmem, dbz2sound_writemem)
	MDRV_CPU_PORTS(dbz2sound_readport,dbz2sound_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static void YM2151_IRQ(int irq)
{
//	printf("2151 IRQ: %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void DBZ2_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
