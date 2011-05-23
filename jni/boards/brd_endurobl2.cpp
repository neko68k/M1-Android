/* enduro bootleg #2 board - based on sharrier driver */

#include "m1snd.h"

#define SH_YM_CLOCK (4000000)
#define SH_Z80_CLOCK (4000000)

static void SH_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;

static struct SEGAPCMinterface segapcm_interface_32k = {
	SEGAPCM_SAMPLE32K,
	BANK_256,
	RGN_SAMP1,		// memory region
	50
};

static struct YM2203interface ym2203_interface =
{
	2,	/* 2 chips */
	4000000,
	{ YM2203_VOL(50,50), YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQHandler }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0xf000, 0xf0ff, SegaPCM_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0xc0, 0xc0, YM2203_status_port_1_r },
	{ 0x40, 0x40, latch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0xc0, 0xc0, YM2203_control_port_1_w },
	{ 0xc1, 0xc1, YM2203_write_port_1_w },
PORT_END

static int er_suffix[] = { 0, 0, 0, 0, 0, -1 };

static void ER_Init(long srate)
{
	m1snd_setCmdSuffixStr(er_suffix);
}

static void ER_StopCmd(int cmda)
{
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0x81);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
}

M1_BOARD_START( endurobl2 )
	MDRV_NAME( "Enduro Racer (bootleg 2)" )
	MDRV_HWDESC( "Z80, YM2203, Sega PCM (BANK_256)" )
	MDRV_DELAYS( 60, 5 )
	MDRV_INIT( ER_Init )
	MDRV_SEND( SH_SendCmd )
	MDRV_STOP( ER_StopCmd )

	MDRV_CPU_ADD(Z80C, SH_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_PORTS(sound_readport, sound_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_32k)
M1_BOARD_END

static void YM_IRQHandler(int irq)	// irqs are ignored
{
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void SH_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
