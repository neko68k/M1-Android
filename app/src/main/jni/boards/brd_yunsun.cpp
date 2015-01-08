#include "m1snd.h"

#define YunSun_Z80_CLOCK (4000000) 

static void YunSun_SendCmd(int cmda, int cmdb);
//static void HCVolume_callback(int v);

static int cmd_latch;

static void YM_IRQHandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
	1,
	4000000,
	{ 100 },
        { YM_IRQHandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,		/* 1 chip */
	{ 1000000 / 132 },	/* frequency (Hz) */
	{ RGN_SAMP1 },	/* memory region */
	{ 50 }
};

static WRITE8_HANDLER( oki_banking_w )
{
	OKIM6295_set_bank_base(0, (data & 1) ? 0x40000 : 0 );
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( ys_readmem )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( ys_writemem )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( ys_readmem2 )
	{ 0x0000, 0xf7ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( ys_writemem2 )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0x10, 0x10, YM3812_status_port_0_r },
	{ 0x18, 0x18, latch_r },
	{ 0x1c, 0x1c, OKIM6295_status_0_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x00, 0x00, oki_banking_w },
	{ 0x10, 0x10, YM3812_control_port_0_w },
	{ 0x11, 0x11, YM3812_write_port_0_w },
	{ 0x1c, 0x1c, OKIM6295_data_0_w },
PORT_END

static void YunSun_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START(yunsun)
	MDRV_NAME( "Yun Sun" )
	MDRV_HWDESC( "Z80, YM3812, MSM-6295" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( YunSun_SendCmd )

	MDRV_CPU_ADD(Z80C, YunSun_Z80_CLOCK)
	MDRV_CPU_MEMORY(ys_readmem, ys_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START(yunsunalt)
	MDRV_NAME( "Yun Sun (alt)" )
	MDRV_HWDESC( "Z80, YM3812, MSM-6295" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( YunSun_SendCmd )

	MDRV_CPU_ADD(Z80C, YunSun_Z80_CLOCK)
	MDRV_CPU_MEMORY(ys_readmem2, ys_writemem2)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

