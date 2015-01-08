/* Video System YM2608 boards */
/* Z80 + YM2608 */

#include "m1snd.h"

static void T2N_Init(long srate);
static void T2N_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static WRITE_HANDLER( sound_bankswitch_w )
{
	cpu_setbank(3,memory_region(REGION_CPU1) + 0x10000 + (data & 0x01) * 0x8000);
}

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct YM2608interface ym2608_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ sound_bankswitch_w },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) }
};

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK3 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x07, 0x07, latch_r },
	{ 0x18, 0x18, YM2608_status_port_0_A_r },
	{ 0x1a, 0x1a, YM2608_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x07, 0x07, IOWP_NOP },	/* clear pending command */
	{ 0x08, 0x08, YM2608_control_port_0_A_w },
	{ 0x09, 0x09, YM2608_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2608_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2608_data_port_0_B_w },
PORT_END

static PORT_READ_START( hatris_sound_readport )
	{ 0x04, 0x04, latch_r },
	{ 0x05, 0x05, IORP_NOP },
	{ 0x08, 0x08, YM2608_status_port_0_A_r },
	{ 0x0a, 0x0a, YM2608_status_port_0_B_r },
PORT_END


static PORT_WRITE_START( hatris_sound_writeport )
	{ 0x02, 0x02, YM2608_control_port_0_B_w },
	{ 0x03, 0x03, YM2608_data_port_0_B_w },
	{ 0x05, 0x05, IOWP_NOP },
	{ 0x08, 0x08, YM2608_control_port_0_A_w },
	{ 0x09, 0x09, YM2608_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2608_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2608_data_port_0_B_w },
PORT_END


M1_BOARD_START( tail2nose )
	MDRV_NAME( "Tail to Nose" )
	MDRV_HWDESC( "Z80, YM2608" )
	MDRV_DELAYS( 60, 20 )
	MDRV_INIT( T2N_Init )
	MDRV_SEND( T2N_SendCmd )

	MDRV_CPU_ADD(Z80C, 14318000/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_SOUND_ADD(YM2608, &ym2608_interface)
M1_BOARD_END

M1_BOARD_START( hatris )
	MDRV_NAME( "Hatris" )
	MDRV_HWDESC( "Z80, YM2608" )
	MDRV_DELAYS( 60, 20 )
	MDRV_INIT( T2N_Init )
	MDRV_SEND( T2N_SendCmd )

	MDRV_CPU_ADD(Z80C, 14318000/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(hatris_sound_readport,hatris_sound_writeport)

	MDRV_SOUND_ADD(YM2608, &ym2608_interface)
M1_BOARD_END

static void T2N_Init(long srate)
{
	cpu_setbank(3,memory_region(REGION_CPU2) + 0x10000);
}

static void T2N_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
