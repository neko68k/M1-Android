// Nichibutsu mid-80s hardware

#include "m1snd.h"
#define TIMER_RATE (1.0/7680.0)

static void Leg_Init(long srate);
static void Leg_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ???? */
	{ YM2203_VOL(40,20), YM2203_VOL(40,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip (no more supported) */
	4000000,        /* 4 MHz */
	{ 50 }         /* (not supported) */
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ 100 }		/* volume */
};

static struct DACinterface dac_interface =
{
	2,	/* 2 channels */
	{ 100,100 },
};

static struct DACinterface cclimbr2_dac_interface =
{
	2,	/* 2 channels */
	{ 40, 40 },
};

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static READ_HANDLER( latch_clear_r )
{
	cmd_latch = 0;
	return 0;
}

static MEMORY_READ_START( soundreadmem )
	{ 0x0000, 0xf7ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( soundwritemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( cclimbr2_soundreadmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( cclimbr2_soundwritemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x4, 0x4, latch_clear_r },
	{ 0x6, 0x6, latch_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x0, 0x0, YM3812_control_port_0_w },
	{ 0x1, 0x1, YM3812_write_port_0_w },
  	{ 0x2, 0x2, DAC_0_signed_data_w },
  	{ 0x3, 0x3, DAC_1_signed_data_w },
PORT_END

M1_BOARD_START( legion )
	MDRV_NAME("Legion")
	MDRV_HWDESC("Z80, YM3812, DAC(x2)")
	MDRV_INIT( Leg_Init )
	MDRV_SEND( Leg_SendCmd )

	MDRV_CPU_ADD(Z80C, 3072000)
	MDRV_CPU_MEMORY(cclimbr2_soundreadmem,cclimbr2_soundwritemem)
	MDRV_CPU_PORTS(readport,writeport)
	
	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(DAC, &cclimbr2_dac_interface)
M1_BOARD_END

M1_BOARD_START( terraf )
	MDRV_NAME("Terra Force")
	MDRV_HWDESC("Z80, YM3812, DAC(x2)")
	MDRV_INIT( Leg_Init )
	MDRV_SEND( Leg_SendCmd )

	MDRV_CPU_ADD(Z80C, 3072000)
	MDRV_CPU_MEMORY(soundreadmem,soundwritemem)
	MDRV_CPU_PORTS(readport,writeport)
	
	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x04, 0x04, latch_clear_r },
	{ 0x06, 0x06, latch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_3526 )
	{ 0x00, 0x00, YM3526_control_port_0_w },
	{ 0x01, 0x01, YM3526_write_port_0_w },
	{ 0x02, 0x02, DAC_0_signed_data_w },
	{ 0x03, 0x03, DAC_1_signed_data_w },
PORT_END

static PORT_WRITE_START( sound_writeport_2203 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x02, 0x02, DAC_0_signed_data_w },
	{ 0x03, 0x03, DAC_1_signed_data_w },
PORT_END

M1_BOARD_START( terrac )
	MDRV_NAME("Terra Cresta (OPL)")
	MDRV_HWDESC("Z80, YM3526, DAC(x2)")
	MDRV_INIT( Leg_Init )
	MDRV_SEND( Leg_SendCmd )

	MDRV_CPU_ADD(Z80C, 3072000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_3526)
	
	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
	MDRV_SOUND_ADD(DAC, &cclimbr2_dac_interface)
M1_BOARD_END

M1_BOARD_START( terrac2 )
	MDRV_NAME("Terra Cresta (OPN)")
	MDRV_HWDESC("Z80, YM2203, DAC(x2)")
	MDRV_INIT( Leg_Init )
	MDRV_SEND( Leg_SendCmd )

	MDRV_CPU_ADD(Z80C, 3072000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_2203)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(DAC, &cclimbr2_dac_interface)
M1_BOARD_END

static void timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void Leg_Init(long srate)
{
	timer_set(TIMER_RATE, 0, timer);

	cmd_latch = 0;
}

static void Leg_SendCmd(int cmda, int cmdb)
{
	cmd_latch = (cmda << 1) | 1;
}
