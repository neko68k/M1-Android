/* test */

#include "m1snd.h"

static int cmd_latch;

static void TV_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x21ff, MRA_RAM },
	{ 0x6000, 0x6000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x21ff, MWA_RAM },
	{ 0x4000, 0x4000, YM2413_register_port_0_w },
	{ 0x4001, 0x4001, YM2413_data_port_0_w },
MEMORY_END

static struct YM2413interface ym2413_interface =
{
	1,
	3579545,
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) },		/* Insufficient gain. */
};

M1_BOARD_START( thunderv )
	MDRV_NAME("ThunderV")			// hardware name
	MDRV_HWDESC("Z80, YM2413")	// hardware description
	MDRV_SEND( TV_SendCmd )		// routine to send a command to the hardware

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)

	MDRV_SOUND_ADD(YM2413, &ym2413_interface)
M1_BOARD_END

