/* Mr. Do Run Run / Mr. Do vs. Unicorns */

#include "m1snd.h"

static unsigned char buffer0[9], buffer1[9];

static void DRR_SendCmd(int cmda, int cmdb)
{
	if (!cmda)
	{
		// I couldn't figure out the proper stop command, so here's the cheat.
		cpu_set_reset_line(0, ASSERT_LINE);
		cpu_set_reset_line(0, CLEAR_LINE);
	
		buffer1[0] = 0x40;
		buffer1[1] = 0xff;
		buffer1[2] = 0x01;
		buffer1[3] = 0xff;
		buffer1[4] = 0x01;
		buffer1[5] = 0xff;
		buffer1[6] = 0x00;
		buffer1[7] = 0x00;
		buffer1[8] = 0x3f;
	}
	else
	{
		buffer1[0] = 0x44;
		buffer1[1] = cmda;	      // 8/4c  7/4b  6/4a
		buffer1[2] = 0x01;
		buffer1[3] = 0x18;
		buffer1[4] = 0x4;
		buffer1[5] = 0x1a;
		buffer1[6] = 0x4;
		buffer1[7] = 0x0;
		buffer1[8] = 0x7f + cmda;
	}
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static struct SN76496interface sn76496_interface =
{
	4,	/* 4 chips */
	{ 4000000, 4000000, 4000000, 4000000 },	/* 4 MHz? */
	{ 25, 25, 25, 25 }
};

static READ_HANDLER( docastle_shared1_r )
{
//	printf("sh1r @ %x = %x\n", offset, buffer1[offset]);
	if (offset == 8)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	}
	return buffer1[offset];
}

static WRITE_HANDLER( docastle_shared0_w )
{
	buffer0[offset] = data;
}

static READ_HANDLER( docastle_flipscreen_off_r )
{
	return 0;
}

static READ_HANDLER( docastle_flipscreen_on_r )
{
	return 1;
}

static MEMORY_READ_START( dorunrun_readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc003, 0xc003, MRA_NOP },
	{ 0xc083, 0xc083, MRA_NOP },
	{ 0xc005, 0xc005, MRA_NOP },
	{ 0xc085, 0xc085, MRA_NOP },
	{ 0xc007, 0xc007, MRA_NOP },
	{ 0xc087, 0xc087, MRA_NOP },
	{ 0xc002, 0xc002, MRA_NOP },
	{ 0xc082, 0xc082, MRA_NOP },
	{ 0xc001, 0xc001, MRA_NOP },
	{ 0xc081, 0xc081, MRA_NOP },
	{ 0xc004, 0xc004, docastle_flipscreen_off_r },
	{ 0xc084, 0xc084, docastle_flipscreen_on_r },
	{ 0xe000, 0xe008, docastle_shared1_r },
MEMORY_END

static MEMORY_WRITE_START( dorunrun_writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, SN76496_0_w },
	{ 0xa400, 0xa400, SN76496_1_w },
	{ 0xa800, 0xa800, SN76496_2_w },
	{ 0xac00, 0xac00, SN76496_3_w },
	{ 0xc004, 0xc004, MWA_NOP },
	{ 0xc084, 0xc084, MWA_NOP },
	{ 0xe000, 0xe008, docastle_shared0_w },
MEMORY_END

static void _timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(1.0/480.0, 0, _timer);
}

static void DRR_Init(long srate)
{
	buffer1[0] = 0x40;
	buffer1[1] = 0xff;
	buffer1[2] = 0x01;
	buffer1[3] = 0xff;
	buffer1[4] = 0x01;
	buffer1[5] = 0xff;
	buffer1[6] = 0x00;
	buffer1[7] = 0x00;
	buffer1[8] = 0x3f;

	timer_set(1.0/480.0, 0, _timer);
}

M1_BOARD_START( dorunrun )
	MDRV_NAME("Do! Run Run")
	MDRV_HWDESC("Z80, SN76496(x4)")
	MDRV_INIT( DRR_Init )
	MDRV_SEND( DRR_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(dorunrun_readmem2,dorunrun_writemem2)

	MDRV_SOUND_ADD(SN76496, &sn76496_interface)
M1_BOARD_END

static MEMORY_READ_START( docastle_readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa008, docastle_shared1_r },
	{ 0xc003, 0xc003, MRA_NOP },
	{ 0xc083, 0xc083, MRA_NOP },
	{ 0xc005, 0xc005, MRA_NOP },
	{ 0xc085, 0xc085, MRA_NOP },
	{ 0xc007, 0xc007, MRA_NOP },
	{ 0xc087, 0xc087, MRA_NOP },
	{ 0xc002, 0xc002, MRA_NOP },
	{ 0xc082, 0xc082, MRA_NOP },
	{ 0xc001, 0xc001, MRA_NOP },
	{ 0xc081, 0xc081, MRA_NOP },
	{ 0xc004, 0xc004, docastle_flipscreen_off_r },
	{ 0xc084, 0xc084, docastle_flipscreen_on_r },
MEMORY_END

static MEMORY_WRITE_START( docastle_writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa008, docastle_shared0_w },
	{ 0xe000, 0xe000, SN76496_0_w },
	{ 0xe400, 0xe400, SN76496_1_w },
	{ 0xe800, 0xe800, SN76496_2_w },
	{ 0xec00, 0xec00, SN76496_3_w },
	{ 0xc004, 0xc004, MWA_NOP },
	{ 0xc084, 0xc084, MWA_NOP },
MEMORY_END

M1_BOARD_START( docastle )
	MDRV_NAME("Mr. Do's Castle")
	MDRV_HWDESC("Z80, SN76496(x4)")
	MDRV_INIT( DRR_Init )
	MDRV_SEND( DRR_SendCmd )
	MDRV_DELAYS(400, 400)

	MDRV_CPU_ADD(Z80C, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(docastle_readmem2,docastle_writemem2)

	MDRV_SOUND_ADD(SN76496, &sn76496_interface)
M1_BOARD_END

