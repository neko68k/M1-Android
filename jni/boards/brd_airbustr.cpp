// Kaneko "Air Buster" (Z80 + YM2203 + MSM6295)

#include "m1snd.h"

static void Abs_Init(long srate);
static void Abs_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static void irq_handler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,
	3000000,					/* ?? */
	{ YM2203_VOL(40,22) },	/* gain,volume */
	{ 0 },			/* DSW-1 connected to port A */
	{ 0 },			/* DSW-2 connected to port B */
	{ 0 },
	{ 0 },
	{ irq_handler }
};

static struct OKIM6295interface m6295_interface =
{
	1,
	{ 12000000/4/165 }, /* 3MHz -> 6295 (mode A) */
	{ RGN_SAMP1 },
	{ 70 }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK3 },
	{ 0xc000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
MEMORY_END

static WRITE_HANDLER( sound_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	RAM = &RAM[0x4000 * (data & 7)];

	cpu_setbank(3, RAM);
}

static PORT_READ_START( readport )
	{ 0x02, 0x02, YM2203_status_port_0_r },
	{ 0x03, 0x03, YM2203_read_port_0_r },
	{ 0x04, 0x04, OKIM6295_status_0_r },
	{ 0x06, 0x06, latch_r },			// read command from sub cpu
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, sound_bankswitch_w },
	{ 0x02, 0x02, YM2203_control_port_0_w },
	{ 0x03, 0x03, YM2203_write_port_0_w },
	{ 0x04, 0x04, OKIM6295_data_0_w },
	{ 0x06, 0x06, IOWP_NOP },		// write command result to sub cpu
PORT_END

M1_BOARD_START( airbuster )
	MDRV_NAME("Air Buster")
	MDRV_HWDESC("Z80, YM2203, MSM-6295")
	MDRV_INIT( Abs_Init )
	MDRV_SEND( Abs_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY( readmem, writemem )
	MDRV_CPU_PORTS( readport, writeport )
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &m6295_interface)
M1_BOARD_END

static void Abs_Init(long srate)
{
	unsigned char *RAM = memory_region(REGION_CPU1) + 0x8000;
	cpu_setbank(3, RAM);
}

static void Abs_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
