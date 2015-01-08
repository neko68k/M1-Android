// Afega h/w (z80 + 2151 + 6295, oldest story in the book)

#include "m1snd.h"

static void Afg_SendCmd(int cmda, int cmdb);
static int cmd_latch;

static void irq_handler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface afega_ym2151_intf =
{
	1,
	4000000,	/* ? */
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct OKIM6295interface afega_m6295_intf =
{
	1,
	{ 8000 },	/* ? */
	{ RGN_SAMP1 },
	{ 70 }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, latch_r },
	{ 0xf809, 0xf809, YM2151_status_port_0_r },
	{ 0xf80a, 0xf80a, OKIM6295_status_0_r },
	{ 0xf900, 0xf900, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, MWA_NOP },
	{ 0xf808, 0xf808, YM2151_register_port_0_w },
	{ 0xf809, 0xf809, YM2151_data_port_0_w },
	{ 0xf80a, 0xf80a, OKIM6295_data_0_w },
	{ 0xf8ff, 0xf8ff, MWA_NOP },
MEMORY_END

M1_BOARD_START( afega )
	MDRV_NAME("Afega")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_SEND( Afg_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY( readmem, writemem )
	
	MDRV_SOUND_ADD(YM2151, &afega_ym2151_intf)
	MDRV_SOUND_ADD(OKIM6295, &afega_m6295_intf)
M1_BOARD_END

static void Afg_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
