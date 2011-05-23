/*
	SNK Beast Busters and Mechanized Attack
*/

#include "m1snd.h"

static void BB_Init(long sr);
static void BB_SendCmd(int cmda, int cmdb);
static void MA_Init(long sr);
static void MA_SendCmd(int cmda, int cmdb);
static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static void sound_irq(int irq)
{
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,
	{ MIXERG(30, MIXER_GAIN_1x, MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ sound_irq },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static struct YM2608interface ym2608_interface =
{
	1,
	8000000,
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ sound_irq },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) }
};

/*
	Common Map
*/
static MEMORY_READ_START( bbusters_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, latch_r },
MEMORY_END

static MEMORY_WRITE_START( bbusters_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
MEMORY_END

/*
	Beast Busters I/O
*/
static PORT_READ_START( bbusters_sound_readport )
	{ 0x00, 0x00, YM2610_status_port_0_A_r },
	{ 0x02, 0x02, YM2610_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( bbusters_sound_writeport )
	{ 0x00, 0x00, YM2610_control_port_0_A_w	},
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w	},
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0xc0, 0xc1, IOWP_NOP },
PORT_END

/*
	Mechanized Attack I/O
*/
static PORT_READ_START( mechatt_sound_readport )
	{ 0x00, 0x00, YM2608_status_port_0_A_r },
	{ 0x02, 0x02, YM2608_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( mechatt_sound_writeport )
	{ 0x00, 0x00, YM2608_control_port_0_A_w	},
	{ 0x01, 0x01, YM2608_data_port_0_A_w },
	{ 0x02, 0x02, YM2608_control_port_0_B_w	},
	{ 0x03, 0x03, YM2608_data_port_0_B_w },
	{ 0xc0, 0xc1, IOWP_NOP },
PORT_END


M1_BOARD_START( bbusters )
	MDRV_NAME("Beast Busters")
	MDRV_HWDESC("Z80, YM2610")
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(bbusters_sound_readmem, bbusters_sound_writemem)
	MDRV_CPU_PORTS(bbusters_sound_readport, bbusters_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &ym2610_interface)
M1_BOARD_END

M1_BOARD_START( mechatt )
	MDRV_NAME("Mechanized Attack")
	MDRV_HWDESC("Z80, YM2608")
	MDRV_INIT( MA_Init )
	MDRV_SEND( MA_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(bbusters_sound_readmem, bbusters_sound_writemem)
	MDRV_CPU_PORTS(mechatt_sound_readport, mechatt_sound_writeport)

	MDRV_SOUND_ADD(YM2608, &ym2608_interface)
M1_BOARD_END

static void BB_Init(long sr)
{
	m1snd_addToCmdQueue(0x04);
}

static void MA_Init(long sr)
{
	m1snd_addToCmdQueue(0x01);
}

static void MA_SendCmd(int cmda, int cmdb)
{	
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void BB_SendCmd(int cmda, int cmdb)
{
	/* Need to drop this range or else the sound effects won't play */
	if ( (cmda > 0x3b) && (cmda < 0x60) )
		return;

	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
