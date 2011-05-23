/* Gradius 3 YM2151 board */

#include "m1snd.h"

#define Z80_CLOCK (3579545)
#define YM_CLOCK (3579545)

static void GRD3_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static void volume_callback(int v);

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ RGN_SAMP1 },	/* memory regions */
	{ K007232_VOL(15,MIXER_PAN_CENTER,15,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static void volume_callback(int v)
{
	K007232_set_volume(0, 0, (v >> 4) * 0x11, 0);
	K007232_set_volume(0, 1, 0, (v & 0x0f) * 0x11);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( sound_bank_w )
{
	int bank_A, bank_B;

	/* banks # for the 007232 (chip 1) */
	bank_A = ((data >> 0) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 0, bank_A, bank_B );
}

static MEMORY_READ_START( gradius3_s_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf010, 0xf010, latch_r },
	{ 0xf020, 0xf02d, K007232_read_port_0_r },
	{ 0xf031, 0xf031, YM2151_status_port_0_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( gradius3_s_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf000, sound_bank_w },				/* 007232 bankswitch */
	{ 0xf020, 0xf02d, K007232_write_port_0_w },
	{ 0xf030, 0xf030, YM2151_register_port_0_w },
	{ 0xf031, 0xf031, YM2151_data_port_0_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

M1_BOARD_START( gradius3 )
	MDRV_NAME( "Gradius 3" )
	MDRV_HWDESC( "Z80, YM2151, K007232" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( GRD3_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(gradius3_s_readmem,gradius3_s_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
M1_BOARD_END

static void GRD3_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
