/* cave and cave-like games with the z80/ym2203/msm6295 sound */

/*

 command info:
 mazinger: must send 0x55, 0xff, 0xfe first to unlock, commands are normal 1-byte after that
 hotdogst: two-byte latch.  one byte is always 0x6d for music, second byte is 0 to stop or 1+ for song number.

*/

#include "m1snd.h"

#define CAVE_Z80_CLOCK (4000000)
#define CAVE_YM_CLOCK (4000000)

static void Hotdog_Init(long srate);
static void CAVE_Init(long srate);
static void CAVE_SendCmd(int cmda, int cmdb);
static void Hotdog_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static READ_HANDLER( latch2_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return 0x6d;
}

static READ_HANDLER( latch3_r )
{
	return cmd_latch;
}

static struct OKIM6295interface okim6295_intf_8kHz =
{
	1,
	{ 8000 },           /* ? */
	{ RGN_SAMP1 },
	{ 100 }
};

static struct YM2203interface ym2203_intf_4MHz =
{
	1,
	4000000,	/* ? */
	{ YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQHandler }
};

static struct YM2203interface ym2203_maz_intf_4MHz =
{
	1,
	4000000,	/* ? */
	{ YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQHandler }
};

WRITE_HANDLER( hotdogst_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU1);
	int bank = data & 0x0f;
	cpu_setbank(2, &RAM[ 0x4000 * bank ]);
}

WRITE_HANDLER( hotdogst_okibank_w )
{
	data8_t *RAM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0x3;
	int bank2 = (data >> 4) & 0x3;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static MEMORY_READ_START( hotdogst_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK2	},	// ROM (Banked)
	{ 0xe000, 0xffff, MRA_RAM	},	// RAM
MEMORY_END

static MEMORY_WRITE_START( hotdogst_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM	},	// ROM (Banked)
	{ 0xe000, 0xffff, MWA_RAM	},	// RAM
MEMORY_END

static PORT_READ_START( hotdogst_sound_readport )
	{ 0x30, 0x30, latch2_r			},	// From Main CPU
	{ 0x40, 0x40, latch3_r			},	//
	{ 0x50, 0x50, YM2203_status_port_0_r	},	// YM2203
	{ 0x51, 0x51, YM2203_read_port_0_r		},	//
	{ 0x60, 0x60, OKIM6295_status_0_r		},	// M6295
PORT_END

static PORT_WRITE_START( hotdogst_sound_writeport )
	{ 0x00, 0x00, hotdogst_rombank_w		},	// ROM bank
	{ 0x50, 0x50, YM2203_control_port_0_w	},	// YM2203
	{ 0x51, 0x51, YM2203_write_port_0_w		},	//
	{ 0x60, 0x60, OKIM6295_data_0_w			},	// M6295
	{ 0x70, 0x70, hotdogst_okibank_w		},	// Samples bank
PORT_END

M1_BOARD_START( hotdog )
	MDRV_NAME("Hotdog Storm")
	MDRV_HWDESC("Z80, YM2203, MSM-6295")
	MDRV_DELAYS( 800, 100 )
	MDRV_INIT( Hotdog_Init )
	MDRV_SEND( Hotdog_SendCmd )

	MDRV_CPU_ADD(Z80C, CAVE_Z80_CLOCK)
	MDRV_CPU_MEMORY(hotdogst_sound_readmem,hotdogst_sound_writemem)
	MDRV_CPU_PORTS(hotdogst_sound_readport,hotdogst_sound_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_intf_4MHz)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_intf_8kHz)
M1_BOARD_END

WRITE_HANDLER( mazinger_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU1);
	int bank = data & 0x07;
	cpu_setbank(2, &RAM[ 0x4000 * bank ]);
}

static MEMORY_READ_START( mazinger_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK2	},	// ROM (Banked)
	{ 0xc000, 0xc7ff, MRA_RAM	},	// RAM
	{ 0xf800, 0xffff, MRA_RAM	},	// RAM
MEMORY_END

static MEMORY_WRITE_START( mazinger_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM	},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM	},	// ROM (Banked)
	{ 0xc000, 0xc7ff, MWA_RAM	},	// RAM
	{ 0xf800, 0xffff, MWA_RAM	},	// RAM
MEMORY_END

static PORT_READ_START( mazinger_sound_readport )
	{ 0x30, 0x30, latch_r			},	// From Main CPU
	{ 0x52, 0x52, YM2203_status_port_0_r	},	// YM2203
PORT_END

static PORT_WRITE_START( mazinger_sound_writeport )
	{ 0x00, 0x00, mazinger_rombank_w		},	// ROM bank
	{ 0x10, 0x10, IOWP_NOP			},	// To Main CPU
	{ 0x50, 0x50, YM2203_control_port_0_w	},	// YM2203
	{ 0x51, 0x51, YM2203_write_port_0_w		},	//
	{ 0x70, 0x70, OKIM6295_data_0_w			},	// M6295
	{ 0x74, 0x74, hotdogst_okibank_w		},	// Samples bank
PORT_END

M1_BOARD_START( mazinger )
	MDRV_NAME("Mazinger Z")
	MDRV_HWDESC("Z80, YM2203, MSM-6295")
	MDRV_DELAYS( 800, 100 )
	MDRV_INIT( CAVE_Init )
	MDRV_SEND( CAVE_SendCmd )

	MDRV_CPU_ADD(Z80C, CAVE_Z80_CLOCK)
	MDRV_CPU_MEMORY(mazinger_sound_readmem,mazinger_sound_writemem)
	MDRV_CPU_PORTS(mazinger_sound_readport,mazinger_sound_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_maz_intf_4MHz)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_intf_8kHz)
M1_BOARD_END

static void YM_IRQHandler(int irq)	// irqs are ignored
{
//	printf("IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void CAVE_Init(long srate)
{
	cmd_latch = 0;

	// the z80 enabler sequence for mazinger
	m1snd_addToCmdQueue((0x55)+1);
	m1snd_addToCmdQueue((0xff)+1);
	m1snd_addToCmdQueue((0xfe)+1);
}

static void Hotdog_Init(long srate)
{
	cmd_latch = 0;
}

static void CAVE_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda-1;	// songs start with zero
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void hotdog_timer(int refcon)
{
	cmd_latch = refcon;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void Hotdog_SendCmd(int cmda, int cmdb)
{
	cmd_latch = 0;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);

	timer_set(TIME_IN_MSEC(30), cmda, hotdog_timer);
}
