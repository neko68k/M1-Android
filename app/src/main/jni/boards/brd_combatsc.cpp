/* Konami "Combat School" */

#include "m1snd.h"

static void CSC_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( combasc_play_w )
{
	UPD7759_start_w(0, data & 2);
}

static WRITE_HANDLER( combasc_voice_reset_w )
{
    UPD7759_reset_w(0,data & 1);
}

static WRITE_HANDLER( combasc_portA_w )
{
}

static struct YM2203interface ym2203_interface =
{
	1,							/* 1 chip */
	3000000,					/* this is wrong but gives the correct music tempo. */
	/* the correct value is 20MHz/8=2.5MHz, which gives correct pitch but wrong tempo */
	{ YM2203_VOL(20,20) },
	{ 0 },
	{ 0 },
	{ combasc_portA_w },
	{ 0 }
};

static struct UPD7759_interface upd7759_interface =
{
	1,							/* number of chips */
	{ 70 },						/* volume */
	{ REGION_SOUND1 },			/* memory region */
	UPD7759_STANDALONE_MODE,	/* chip mode */
	{0}
};

static MEMORY_READ_START( combasc_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },					/* ROM */
	{ 0x8000, 0x87ff, MRA_RAM },					/* RAM */
	{ 0xb000, 0xb000, UPD7759_0_busy_r },			/* UPD7759 busy? */
	{ 0xd000, 0xd000, latch_r },				/* soundlatch_r? */
    	{ 0xe000, 0xe000, YM2203_status_port_0_r },		/* YM 2203 */
MEMORY_END

static MEMORY_WRITE_START( combasc_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },				/* ROM */
	{ 0x8000, 0x87ff, MWA_RAM },				/* RAM */
	{ 0x9000, 0x9000, combasc_play_w },			/* uPD7759 play voice */
	{ 0xa000, 0xa000, UPD7759_0_port_w },		/* uPD7759 voice select */
	{ 0xc000, 0xc000, combasc_voice_reset_w },	/* uPD7759 reset? */
 	{ 0xe000, 0xe000, YM2203_control_port_0_w },/* YM 2203 */
	{ 0xe001, 0xe001, YM2203_write_port_0_w },	/* YM 2203 */
MEMORY_END
M1_BOARD_START( combatsc )
	MDRV_NAME("Combat School")
	MDRV_HWDESC("Z80, YM2203, uPD7759")
	MDRV_SEND( CSC_SendCmd )

	MDRV_CPU_ADD(Z80C, 1500000)
	MDRV_CPU_MEMORY(combasc_readmem_sound,combasc_writemem_sound)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END

static void CSC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
