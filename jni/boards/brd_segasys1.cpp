/* Sega System 1 and 2 */

#include "m1snd.h"

#define SS1_YM_CLOCK (1500000)
#define SS1_Z80_CLOCK (3000000)

static void SS1_Init(long srate);
static void SS1_SendCmd(int cmda, int cmdb);

#define TIMER_RATE (1.0/240.0)	// 4 times per 60 hz frame

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct SN76496interface sn76496_interface =
{
	2,      /* 2 chips */
	{ 2000000, 4000000 },   /* 8 MHz / 4 ?*/
	{ 50, 50 }
};

static unsigned char *work_ram;

static READ_HANDLER( work_ram_r )
{
	return work_ram[offset];
}

static WRITE_HANDLER( work_ram_w )
{
	work_ram[offset] = data;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, work_ram_r },
	{ 0x8800, 0x8fff, work_ram_r },
	{ 0xe000, 0xe000, latch_r },
	{ 0xffff, 0xffff, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, work_ram_w, &work_ram },
	{ 0x8800, 0x8fff, work_ram_w },
	{ 0xa000, 0xa003, SN76496_0_w },
	{ 0xc000, 0xc003, SN76496_1_w },
MEMORY_END

M1_BOARD_START( segasys1 )
	MDRV_NAME("Sega System 1/2")
	MDRV_HWDESC("Z80, SN76496")
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( SS1_Init )
	MDRV_SEND( SS1_SendCmd )

	MDRV_CPU_ADD(Z80C, SS1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(SN76496, &sn76496_interface)
M1_BOARD_END

static void timer(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void SS1_Init(long srate)
{
	timer_set(TIMER_RATE, 0, timer);
}

static void SS1_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
