/*
	Taito sound board 'J1100005A / K1100011A'
	- Z80, MSM5232, AY-3-8910 and DAC

	Note:
	TA7630 emulation needs filter support (bass sounds from
	MSM5232 should be about 2 times louder)
*/
#include <math.h>
#include "m1snd.h"

static void Taito84_Init(long srate);
static void MSIsaac_Init(long srate);
static void Taito84_SendCmd(int cmda, int cmdb);

static int cmd_latch, sound_nmi_enable, pending_nmi;
static void *periodic_timer;
static UINT8 snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3;
static int vol_ctrl[16];


static WRITE_HANDLER( msi_sound_control_0_w )
{
	snd_ctrl0 = data & 0xff;

	mixer_set_volume (6, vol_ctrl[snd_ctrl0     & 15]);		/* Group 1 from MSM5232 */
	mixer_set_volume (7, vol_ctrl[(snd_ctrl0>>4) & 15]);	/* Group 2 from MSM5232 */

}
static WRITE_HANDLER( msi_sound_control_1_w )
{
	snd_ctrl1 = data & 0xff;
}



static WRITE_HANDLER( sound_control_0_w )
{
	snd_ctrl0 = data & 0xff;
	/* This definitely controls main melody voice on 2'-1 and 4'-1 outputs */
	mixer_set_volume (3, vol_ctrl[ (snd_ctrl0>>4) & 15 ]);	/* Group 1 from MSM5232 */

}
static WRITE_HANDLER( sound_control_1_w )
{
	snd_ctrl1 = data & 0xff;
	mixer_set_volume (4, vol_ctrl[ (snd_ctrl1>>4) & 15 ]);	/* Group 2 from MSM5232 */
}

static WRITE_HANDLER( sound_control_2_w )
{
	int i;

	snd_ctrl2 = data & 0xff;

	for (i=0; i<3; i++)
		mixer_set_volume (i, vol_ctrl[ (snd_ctrl2>>4) & 15 ]);	/* YM2149f all */
}

static WRITE_HANDLER( sound_control_3_w )
{
	/* Unknown */
	snd_ctrl3 = data & 0xff;
}


static WRITE_HANDLER( nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE_HANDLER( nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
		pending_nmi = 0;
	}
}


static struct AY8910interface ay8910_interface =
{
	1,
	8000000/4,
	{ MIXER(10, MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ sound_control_2_w },
	{ sound_control_3_w }
};

static struct MSM5232interface msm5232_interface =
{
	1,
	2000000,
	{ { 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6 } },	/* 1.0 uF capacitors (verified on real PCB) */
	{ MIXER(100, MIXER_PAN_CENTER) }
};

static struct DACinterface dac_interface =
{
	1,
	{ MIXER(20, MIXER_PAN_CENTER) }
};



static struct AY8910interface msisaac_ay8910_interface =
{
	2,
	2000000,
	{ MIXER(15, MIXER_PAN_CENTER), MIXER(15, MIXER_PAN_CENTER) },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 }
};

static struct MSM5232interface msisaac_msm5232_interface =
{
	1,
	2000000,
	{ { 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6 } },	/* 0.65 (???) uF capacitors (match the sample, not verified) */
	{ MIXER(100, MIXER_PAN_CENTER) }
};


static READ8_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}


static MEMORY_WRITE_START( taito84_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, AY8910_control_port_0_w },
	{ 0xc801, 0xc801, AY8910_write_port_0_w },
	{ 0xca00, 0xca0d, MSM5232_0_w },
	{ 0xcc00, 0xcc00, sound_control_0_w },
	{ 0xce00, 0xce00, sound_control_1_w },
	{ 0xd800, 0xd800, MWA_NOP },
	{ 0xda00, 0xda00, nmi_enable_w },
	{ 0xdc00, 0xdc00, nmi_disable_w },
	{ 0xde00, 0xde00, DAC_0_signed_data_w },
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( taito84_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd800, 0xd800, latch_r },
	{ 0xda00, 0xda00, MRA_NOP },
	{ 0xde00, 0xde00, MRA_NOP },
	{ 0xe000, 0xefff, MRA_ROM },
MEMORY_END

M1_BOARD_START( taito84 )
	MDRV_NAME("Taito Sound Board '84")
	MDRV_HWDESC("Z80, MSM5232, AY-3-8910, DAC")
	MDRV_INIT( Taito84_Init )
	MDRV_SEND( Taito84_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000/2)
	MDRV_CPU_MEMORY(taito84_readmem, taito84_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, &msm5232_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END


static MEMORY_READ_START( msisaac_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xc000, 0xc000, latch_r },
	{ 0xe000, 0xffff, MRA_NOP },	/* Space for diagnostic ROM (not dumped, not reachable) */
MEMORY_END

static MEMORY_WRITE_START( msisaac_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0x8002, 0x8002, AY8910_control_port_1_w },
	{ 0x8003, 0x8003, AY8910_write_port_1_w },
	{ 0x8010, 0x801d, MSM5232_0_w },
	{ 0x8020, 0x8020, msi_sound_control_0_w },
	{ 0x8030, 0x8030, msi_sound_control_1_w },
	{ 0xc001, 0xc001, nmi_enable_w },
	{ 0xc002, 0xc002, nmi_disable_w },
	{ 0xc003, 0xc003, MWA_NOP },	/* This is NOT mixer_enable */
MEMORY_END


M1_BOARD_START( msisaac )
	MDRV_NAME("Metal Soldier Isaac")
	MDRV_HWDESC("Z80, MSM5232, AY-3-8910(x2)")
	MDRV_INIT( MSIsaac_Init )
	MDRV_SEND( Taito84_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000/2)
	MDRV_CPU_MEMORY(msisaac_readmem, msisaac_writemem)

	MDRV_SOUND_ADD(AY8910, &msisaac_ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, &msisaac_msm5232_interface)
M1_BOARD_END


static void timer_callback(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


static void TA7630_Init(void)
{
	int i;
	double db			= 0.0;
	double db_step		= 0.50;	/* 0.50 dB step (at least, maybe more) */
	double db_step_inc	= 0.275;

	/*
		channels 0-2 AY#0
		channels 3-5 AY#1
		channels 6,7 MSM5232 group1,group2
	*/
	for (i=0; i<16; i++)
	{
		double max = 100.0 / pow(10.0, db/20.0 );
		vol_ctrl[ 15-i ] = (int)max;
		db += db_step;
		db_step += db_step_inc;
	}
}

static void Taito84_Init(long srate)
{
	periodic_timer = timer_pulse(TIME_IN_HZ(120), 0, timer_callback);
	TA7630_Init();

	m1snd_addToCmdQueueRaw(0xef);
}
static void MSIsaac_Init(long srate)
{
	periodic_timer = timer_pulse(TIME_IN_HZ(60), 0, timer_callback);
	TA7630_Init();
}

static void Taito84_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	if (sound_nmi_enable)
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
