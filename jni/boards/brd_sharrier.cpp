/* Sega Space Harrier */

#include "m1snd.h"

static void SH_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;

static struct SEGAPCMinterface segapcm_interface_32k = {
	SEGAPCM_SAMPLE32K,
	BANK_256,
	RGN_SAMP1,		// memory region
	100
};

static struct YM2203interface ym2203_interface =
{
	1,	/* 1 chips */
	4000000,
	{ YM2203_VOL(28,15) },
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
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe0ff, SegaPCM_r },
	{ 0xe100, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x8000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe0ff, SegaPCM_w },
	{ 0xe100, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x40, 0x40, latch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
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

M1_BOARD_START( segasharrier )
	MDRV_NAME( "Space Harrier" )
	MDRV_HWDESC( "Z80, YM2203, Sega PCM (BANK_256)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( SH_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_PORTS(sound_readport, sound_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_32k)
M1_BOARD_END

M1_BOARD_START( erver2 )
	MDRV_NAME( "Enduro Racer (ver. 2)" )
	MDRV_HWDESC( "Z80, YM2203, Sega PCM (BANK_256)" )
	MDRV_DELAYS( 60, 5 )
	MDRV_SEND( SH_SendCmd )
	MDRV_INIT(ER_Init)
	MDRV_STOP(ER_StopCmd)

	MDRV_CPU_ADD(Z80C, 4000000)
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

