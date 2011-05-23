/* Jaleco Mega System 32 

 memory map:
 0000-3eff: program ROM (fixed)
 3f00-3f0f: YMF271-F
 3f10     : command latch (bidirectional)
 8000     : banked ROM area?

 IRQ is unused
 NMI reads the command latch

*/

#include "m1snd.h"

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);

	if (Machine->refcon == 1)
	{
		return cmd_latch ^ 0xff;	// it's tricky!
	}

	return cmd_latch;
}

static void MS32_Init(long srate)
{
	cpu_setbank(4, prgrom + 0x4000);
	cpu_setbank(5, prgrom + 0x8000);

	m1snd_addToCmdQueue(0xff);
	m1snd_addToCmdQueue(0xff);

	if (Machine->refcon == 1)
	{
		m1snd_setCmdPrefix(0x70);
	}
	else
	{
		m1snd_setCmdPrefix(0);
	}
}

static void MS32_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static WRITE_HANDLER( ms32_snd_bank_w )
{
	cpu_setbank(4, memory_region(REGION_CPU1) + 0x4000+(0x4000*(data&0xf)));
	cpu_setbank(5, memory_region(REGION_CPU1) + 0x4000+(0x4000*(data>>4)));
}

static MEMORY_READ_START( ms32_snd_readmem )
	{ 0x0000, 0x3eff, MRA_ROM },
	{ 0x3f00, 0x3f0f, YMF271_0_r },
	{ 0x3f10, 0x3f10, latch_r },		
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_BANK4 },
	{ 0xc000, 0xffff, MRA_BANK5 },
MEMORY_END

static MEMORY_WRITE_START( ms32_snd_writemem )
	{ 0x0000, 0x3eff, MWA_ROM },
	{ 0x3f00, 0x3f0f, YMF271_0_w },
	{ 0x3f10, 0x3f10, MWA_NOP },	// output latch to V70
	{ 0x3f70, 0x3f70, MWA_NOP },	// watchdog?
	{ 0x3f80, 0x3f80, ms32_snd_bank_w },
	{ 0x4000, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static struct YMF271interface ymf271_interface =
{
	1,
	NULL,
	NULL,
	{ REGION_SOUND1, },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT),},
	{ 0 },
};

M1_BOARD_START( ms32 )
	MDRV_NAME("Jaleco Mega System 32")
	MDRV_HWDESC("Z80, YMF271-F (OPX)")
	MDRV_DELAYS(2000, 90)
	MDRV_INIT( MS32_Init )
	MDRV_SEND( MS32_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)	// ???
	MDRV_CPU_MEMORY(ms32_snd_readmem, ms32_snd_writemem)

	MDRV_SOUND_ADD(YMF271, &ymf271_interface)
M1_BOARD_END

