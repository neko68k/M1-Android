/* Capcom dual-YM2203 sound boards */

#include "m1snd.h"

static void GNG_Init(long srate);
static void GNG_SendCmd(int cmda, int cmdb);
static void BT_SendCmd(int cmda, int cmdb);

#define TIMER_RATE (1.0/240.0)	// 4 times per 60 hz frame

static int cmd_latch;

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
	{ YM2203_VOL(20,40), YM2203_VOL(20,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2203interface saym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz ? */
	{ YM2203_VOL(15,25), YM2203_VOL(15,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static READ_HANDLER( latch_r )
{
	return cmd_latch & 0xff;
}

static READ_HANDLER( latch2_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, latch_r },
	{ 0xe006, 0xe007, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xe006, 0xe007, MWA_RAM },
MEMORY_END

M1_BOARD_START( gng )
	MDRV_NAME( "Ghosts and Goblins" )
	MDRV_HWDESC( "Z80, YM2203(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( GNG_Init )
	MDRV_SEND( GNG_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static MEMORY_READ_START( bt_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, latch2_r },
	{ 0xe000, 0xe000, YM2203_status_port_0_r },
	{ 0xe001, 0xe001, YM2203_read_port_0_r },
	{ 0xe002, 0xe002, YM2203_status_port_1_r },
	{ 0xe003, 0xe003, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( bt_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
MEMORY_END

static void irqhandler(int irq)
{
	cpu_set_irq_line(0,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface bt_ym2203_interface =
{
	2,			/* 2 chips */
	3579545,	/* 3.579 MHz ? (hand tuned) */
	{ YM2203_VOL(15,15), YM2203_VOL(15,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

M1_BOARD_START( blktiger )
	MDRV_NAME( "Black Tiger" )
	MDRV_HWDESC( "Z80, YM2203(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( BT_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(bt_sound_readmem, bt_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &bt_ym2203_interface)
M1_BOARD_END

static MEMORY_READ_START( sasound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, latch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( sasound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
MEMORY_END

M1_BOARD_START( sidearms )
	MDRV_NAME( "Side Arms" )
	MDRV_HWDESC( "Z80, YM2203(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( GNG_Init )
	MDRV_SEND( GNG_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(sasound_readmem, sasound_writemem)

	MDRV_SOUND_ADD(YM2203, &saym2203_interface)
M1_BOARD_END

static MEMORY_READ_START( c_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( c_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0x8002, 0x8002, YM2203_control_port_1_w },
	{ 0x8003, 0x8003, YM2203_write_port_1_w },
MEMORY_END

M1_BOARD_START( commando )
	MDRV_NAME( "Commando" )
	MDRV_HWDESC( "Z80, YM2203(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( GNG_Init )
	MDRV_SEND( GNG_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(c_sound_readmem, c_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static struct OKIM6295interface okim6295_interface =
{
	1,              	/* 1 chip */
	{ 7759 },           /* 7759Hz frequency */
	{ REGION_SOUND1 },	/* memory region 3 */
	{ 98 }
};

static struct YM2203interface ld_ym2203_interface =
{
	2,			/* 2 chips */
	3579545, /* Accurate */
	{ YM2203_VOL(40,40), YM2203_VOL(40,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static MEMORY_READ_START( ld_sound_readmem )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xe800, YM2203_status_port_0_r },
	{ 0xf000, 0xf000, YM2203_status_port_1_r },
	{ 0xf800, 0xf800, latch_r },
MEMORY_END

static MEMORY_WRITE_START( ld_sound_writemem )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xe800, YM2203_control_port_0_w },
	{ 0xe801, 0xe801, YM2203_write_port_0_w },
	{ 0xf000, 0xf000, YM2203_control_port_1_w },
	{ 0xf001, 0xf001, YM2203_write_port_1_w },
MEMORY_END

M1_BOARD_START( lastduel )
	MDRV_NAME( "Last Duel" )
	MDRV_HWDESC( "Z80, YM2203(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( BT_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(ld_sound_readmem, ld_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ld_ym2203_interface)
M1_BOARD_END

static WRITE_HANDLER( mg_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = ((data & 0x01) * 0x4000) + 0x8000;
	cpu_setbank(3,&RAM[bankaddress]);
}

static MEMORY_READ_START( mg_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xcfff, MRA_BANK3 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
	{ 0xf006, 0xf006, latch_r },
MEMORY_END

static MEMORY_WRITE_START( mg_sound_writemem )
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
	{ 0xf004, 0xf004, OKIM6295_data_0_w },
	{ 0xf00a, 0xf00a, mg_bankswitch_w },
MEMORY_END

M1_BOARD_START( madgear )
	MDRV_NAME( "Mad Gear" )
	MDRV_HWDESC( "Z80, YM2203(x2), MSM-6295" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( BT_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(mg_sound_readmem, mg_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ld_ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static void timer(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void GNG_Init(long srate)
{
	timer_set(TIMER_RATE, 0, timer);
}

static void GNG_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}

static void BT_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

