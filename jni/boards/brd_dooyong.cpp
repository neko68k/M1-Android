// Dooyong early-90s vertical shooters

#include "m1snd.h"

static void PollSendCmd(int cmda, int cmdb);

static int cmd_latch;

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static READ_HANDLER( unk_r )
{
//	printf("Unknown 2203 port read (PC=%x)\n", z80c_get_reg(REG_PC));
	return cmd_latch;
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz ? */
	{ YM2203_VOL(40,40), YM2203_VOL(40,40) },
	{ unk_r, unk_r },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler, irqhandler }
};

static MEMORY_READ_START( lastday_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, latch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf001, 0xf001, YM2203_read_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
	{ 0xf003, 0xf003, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( lastday_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
MEMORY_END

static MEMORY_READ_START( pollux_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, latch_r },
	{ 0xf802, 0xf802, YM2203_status_port_0_r },
	{ 0xf803, 0xf803, YM2203_read_port_0_r },
	{ 0xf804, 0xf804, YM2203_status_port_1_r },
	{ 0xf805, 0xf805, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( pollux_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf802, 0xf802, YM2203_control_port_0_w },
	{ 0xf803, 0xf803, YM2203_write_port_0_w },
	{ 0xf804, 0xf804, YM2203_control_port_1_w },
	{ 0xf805, 0xf805, YM2203_write_port_1_w },
MEMORY_END

M1_BOARD_START( pollux )
	MDRV_NAME("Pollux")
	MDRV_HWDESC("Z80, YM2203(x2)")
	MDRV_SEND( PollSendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(pollux_sound_readmem,pollux_sound_writemem)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

M1_BOARD_START( lastday )
	MDRV_NAME("The Last Day")
	MDRV_HWDESC("Z80, YM2203(x2)")
	MDRV_SEND( PollSendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(lastday_sound_readmem,lastday_sound_writemem)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static void PollSendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
