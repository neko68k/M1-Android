/* Semicom "Hatch Catch" */

#include "m1snd.h"

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static void HTC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void semicom_irqhandler(int irq)
{
	cpu_set_irq_line(0,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2151interface semicom_ym2151_interface =
{
	1,
	3427190,	/* verified */
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ semicom_irqhandler }
};

static struct OKIM6295interface semicom_okim6295_interface =
{
	1,			/* 1 chip */
	{ 1024000/132 },		/* verified */
	{ REGION_SOUND1 },
	{ 100 }
};

static MEMORY_READ_START( semicom_sound_readmem )
	{ 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xefff, MRA_RAM },
	{ 0xf001, 0xf001, YM2151_status_port_0_r },
	{ 0xf008, 0xf008, latch_r },
MEMORY_END

static MEMORY_WRITE_START( semicom_sound_writemem )
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xefff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
//	{ 0xf006, 0xf006,  }, ???
MEMORY_END

#if 0
M1_ROMS_START( hatchcatch )
	{
		"Hatch Catch",
		"1995",
		"",
		"htchctch",
		MFG_SEMICOM,
		BOARD_HATCHC,
		1,
		0,
		ROM_START
			ROM_REGION( 0x10000, RGN_CPU1, 0 )
			ROM_LOAD( "p02.b5", 0x00000, 0x10000 , CRC(c5a03186) SHA1(42561ab36e6d7a43828d3094e64bd1229ab893ba) )

			ROM_REGION( 0x80000, RGN_SAMP1, 0 )
			ROM_LOAD( "p01.c1", 0x00000, 0x20000, CRC(18c06829) SHA1(46b180319ed33abeaba70d2cc61f17639e59bfdb) )
		ROM_END
		1, 255,
		
	},
M1_ROMS_END
#endif 

M1_BOARD_START( hatchcatch )
	MDRV_NAME("Hatch Catch")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_SEND( HTC_SendCmd )

	MDRV_CPU_ADD(Z80C, 3427190)
	MDRV_CPU_MEMORY(semicom_sound_readmem,semicom_sound_writemem)

	MDRV_SOUND_ADD(YM2151, &semicom_ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &semicom_okim6295_interface)
M1_BOARD_END

