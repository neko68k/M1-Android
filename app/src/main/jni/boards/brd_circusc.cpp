/* Konami's Circus Charlie */

#include "m1snd.h"

#define TIMER_RATE (1.0/6144.0)

static void CC_Init(long srate);
static void CC_SendCmd(int cmda, int cmdb);

static int cmd_latch, curtime;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static void gx400_timer(int refcon)
{
	curtime++;
	curtime &= 0xf;

	timer_set(TIMER_RATE, 0, gx400_timer);
}

static READ_HANDLER( circusc_sh_timer_r )
{
	return curtime;
}

static WRITE_HANDLER( circusc_dac_w )
{
	DAC_data_w(0,data);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, latch_r },
	{ 0x8000, 0x8000, circusc_sh_timer_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_NOP },    /* latch command for the 76496. We should buffer this */
									/* command and send it to the chip, but we just use */
									/* the triggers below because the program always writes */
									/* the same number here and there. */
	{ 0xa001, 0xa001, SN76496_0_w },        /* trigger the 76496 to read the latch */
	{ 0xa002, 0xa002, SN76496_1_w },        /* trigger the 76496 to read the latch */
	{ 0xa003, 0xa003, circusc_dac_w },
	{ 0xa004, 0xa004, MWA_NOP },            /* ??? */
	{ 0xa07c, 0xa07c, MWA_NOP },            /* ??? */
MEMORY_END

static struct SN76496interface sn76496_interface =
{
	2,      /* 2 chips */
	{ 14318180/8, 14318180/8 },     /*  1.7897725 MHz */
	{ 100, 100 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

M1_BOARD_START( circusc )
	MDRV_NAME("Circus Charlie")
	MDRV_HWDESC("Z80, SN76496(x2), DAC")
	MDRV_INIT( CC_Init )
	MDRV_SEND( CC_SendCmd )

	MDRV_CPU_ADD(Z80C, 14318180/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(SN76496, &sn76496_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static void CC_Init(long srate)
{
	timer_set(TIMER_RATE, 0, gx400_timer);
}

static void CC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
