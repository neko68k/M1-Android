// Jaleco Field Combat (similar to Exerion w/a separate sound CPU)

#include "m1snd.h"

static void FCom_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 12, 12, 12 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static MEMORY_READ_START( fcombat_readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, latch_r },
	{ 0x8001, 0x8001, AY8910_read_port_0_r },
	{ 0xa001, 0xa001, AY8910_read_port_1_r },
	{ 0xc001, 0xc001, AY8910_read_port_2_r },
MEMORY_END

static MEMORY_WRITE_START( fcombat_writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8002, 0x8002, AY8910_write_port_0_w },
	{ 0x8003, 0x8003, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_1_w },
	{ 0xa003, 0xa003, AY8910_control_port_1_w },
	{ 0xc002, 0xc002, AY8910_write_port_2_w },
	{ 0xc003, 0xc003, AY8910_control_port_2_w },
MEMORY_END

M1_BOARD_START( fcombat )
	MDRV_NAME("Field Combat")
	MDRV_HWDESC("Z80, YM2149(x3)")
	MDRV_SEND( FCom_SendCmd )

	MDRV_CPU_ADD(Z80C, 10000000/3)
	MDRV_CPU_MEMORY(fcombat_readmem2,fcombat_writemem2)
	
	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END

static void FCom_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
