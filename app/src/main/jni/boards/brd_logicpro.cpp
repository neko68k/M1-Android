/* Logic Pro */

#include "m1snd.h"

static int cmd_latch;

static void LPro_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( deniam16b_oki_rom_bank_w )
{
	OKIM6295_set_bank_base(0,(data & 0x40) ? 0x40000 : 0x00000);
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0xf7ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0x01, 0x01, latch_r },
	{ 0x05, 0x05, OKIM6295_status_0_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x02, 0x02, YM3812_control_port_0_w },
	{ 0x03, 0x03, YM3812_write_port_0_w },
	{ 0x05, 0x05, OKIM6295_data_0_w },
	{ 0x07, 0x07, deniam16b_oki_rom_bank_w },
PORT_END

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	25000000/8,	/* ??? */
	{ 60 },	/* volume */
	{ irqhandler },
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }		    /* volume */
};

M1_BOARD_START( deniam )
	MDRV_NAME("Deniam-16")			// hardware name
	MDRV_HWDESC("Z80, YM2151, MSM-6295")	// hardware description
	MDRV_SEND( LPro_SendCmd )		// routine to send a command to the hardware

	MDRV_CPU_ADD(Z80C, 25000000/4)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

