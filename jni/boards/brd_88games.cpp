/* Konami 88games */

#include "m1snd.h"

#define Z80_CLOCK (3579580)

static void E8_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct UPD7759_interface upd7759_interface =
{
	2,		     		/* number of chips */
	{ 30, 30 },	     		/* volume */
	{ RGN_SAMP1, RGN_SAMP2 },	/* memory region */
	UPD7759_STANDALONE_MODE,	/* chip mode */
	{0}
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch&0xff;
}

static int speech_chip;
static WRITE_HANDLER( speech_msg_w )
{
	UPD7759_port_w( speech_chip, data );
}

static WRITE_HANDLER( speech_control_w )
{
	speech_chip = ( data & 4 ) ? 1 : 0;

	UPD7759_reset_w( speech_chip, data & 2 );
	UPD7759_start_w( speech_chip, data & 1 );
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, latch_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, speech_msg_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xe000, 0xe000, speech_control_w },
MEMORY_END

M1_BOARD_START( 88games )
	MDRV_NAME( "'88 Games" )
	MDRV_HWDESC( "Z80, YM2151, uPD7759(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( E8_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END

static void E8_SendCmd(int cmda, int cmdb)
{
	if (!cmda) return;

	cmd_latch = cmda;

	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

