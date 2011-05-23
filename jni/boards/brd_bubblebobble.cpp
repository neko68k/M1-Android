/* Taito Bubble Bobble board */

#include "m1snd.h"

#define YM_CLOCK (3000000)
#define Z80_CLOCK (3000000)

static void BB_Init(long srate);
static void BB_SendCmd(int cmda, int cmdb);
static void irqhandler(int irq);
static int cmd_latch;
static int sound_nmi_enable,pending_nmi;

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	YM_CLOCK,	/* 3 MHz */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM2203interface ym2203_interface_kage =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz */
	{ YM2203_VOL(50,50), YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	YM_CLOCK,	/* 3 MHz */
	{ 50 },		/* volume */
	{ irqhandler },
};

static struct YM2203interface tokio_ym2203_interface =
{
	1,		/* 1 chip */
	YM_CLOCK,	/* 3 MHz */
	{ YM2203_VOL(100,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

WRITE_HANDLER( bublbobl_sh_nmi_disable_w )
{
	sound_nmi_enable = 0;
}

WRITE_HANDLER( bublbobl_sh_nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
		pending_nmi = 0;
	}
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
	{ 0xa000, 0xa000, YM3526_status_port_0_r },
	{ 0xb000, 0xb000, latch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* bit 0: message pending for main cpu */
					/* bit 1: message pending for sound cpu */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM3526_control_port_0_w },
	{ 0xa001, 0xa001, YM3526_write_port_0_w },
	{ 0xb000, 0xb000, MWA_NOP },	/* message for main cpu */
	{ 0xb001, 0xb001, bublbobl_sh_nmi_enable_w },
	{ 0xb002, 0xb002, bublbobl_sh_nmi_disable_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_READ_START( sound_readmem_kage )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
	{ 0xa000, 0xa000, YM2203_status_port_1_r },
	{ 0xa001, 0xa001, YM2203_read_port_1_r },
	{ 0xb000, 0xb000, latch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* bit 0: message pending for main cpu */
					/* bit 1: message pending for sound cpu */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_kage )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0xb000, 0xb000, MWA_NOP },	/* message for main cpu */
	{ 0xb001, 0xb001, bublbobl_sh_nmi_enable_w },
	{ 0xb002, 0xb002, bublbobl_sh_nmi_disable_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

M1_BOARD_START( bubblebobble )
	MDRV_NAME( "Bubble Bobble" )
	MDRV_HWDESC( "Z80, YM2203, YM3526" )
	MDRV_DELAYS( 60, 250 )
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
M1_BOARD_END

M1_BOARD_START( lkage )
	MDRV_NAME( "Legend of Kage" )
	MDRV_HWDESC( "Z80, YM2203(x2)" )
	MDRV_DELAYS( 60, 250 )
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(sound_readmem_kage,sound_writemem_kage)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface_kage)
M1_BOARD_END

static MEMORY_READ_START( tokio_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, latch_r },
	{ 0x9800, 0x9800, MRA_NOP },	/* ??? */
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( tokio_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, MWA_NOP },	/* ??? */
	{ 0xa000, 0xa000, bublbobl_sh_nmi_disable_w },
	{ 0xa800, 0xa800, bublbobl_sh_nmi_enable_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

M1_BOARD_START( tokio )
	MDRV_NAME( "Tokio" )
	MDRV_HWDESC( "Z80, YM2203" )
	MDRV_DELAYS( 60, 150 )
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(tokio_sound_readmem,tokio_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &tokio_ym2203_interface)
M1_BOARD_END

static void irqhandler(int irq)
{
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void timer_irq(int refcon);

static void timer_irq2(int refcon)
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	timer_set(1.0/120.0, 0, timer_irq);
}

static void timer_irq(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	timer_set(1.0/120.0, 0, timer_irq2);
}

static void BB_Init(long srate)
{
	// send the enabler command
	m1snd_addToCmdQueue(0xef);

	timer_set(1.0/60.0, 0, timer_irq);
}

static void BB_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	if (sound_nmi_enable) cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE); 
	else pending_nmi = 1;
}
