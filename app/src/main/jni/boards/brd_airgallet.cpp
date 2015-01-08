/* Banpresto / Gazelle / Cave "Air Gallet" */

#include "m1snd.h"

static int cmd_latch;

static data8_t *mirror_ram;
static READ_HANDLER( mirror_ram_r )
{
	return mirror_ram[offset];
}
static WRITE_HANDLER( mirror_ram_w )
{
	mirror_ram[offset] = data;
}

static READ_HANDLER( soundlatch_lo_r )
{
	return cmd_latch & 0xff;
}

/* Sound CPU: read the high 8 bits of the 16 bit sound latch */
static READ_HANDLER( soundlatch_hi_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch >> 8;
}

static READ_HANDLER( soundflags_r )
{
	return 0;
}

WRITE_HANDLER( sailormn_rombank_w )
{
	data8_t *RAM = memory_region(REGION_CPU1);
	int bank = data & 0x1f;
	if ( data & ~0x1f )	logerror((char *)"CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	cpu_setbank(1, &RAM[ 0x4000 * bank ]);
}

WRITE_HANDLER( sailormn_okibank0_w )
{
	data8_t *RAM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0xf;
	int bank2 = (data >> 4) & 0xf;
	if (Machine->sample_rate == 0)	return;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

WRITE_HANDLER( sailormn_okibank1_w )
{
	data8_t *RAM = memory_region(REGION_SOUND2);
	int bank1 = (data >> 0) & 0xf;
	int bank2 = (data >> 4) & 0xf;
	if (Machine->sample_rate == 0)	return;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static MEMORY_READ_START( sailormn_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM					},	// ROM
	{ 0x4000, 0x7fff, MRA_BANK1					},	// ROM (Banked)
	{ 0xc000, 0xdfff, mirror_ram_r				},	// RAM
	{ 0xe000, 0xffff, mirror_ram_r				},	// Mirrored RAM (agallet)
MEMORY_END

static MEMORY_WRITE_START( sailormn_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM					},	// ROM
	{ 0x4000, 0x7fff, MWA_ROM					},	// ROM (Banked)
	{ 0xc000, 0xdfff, mirror_ram_w, &mirror_ram	},	// RAM
	{ 0xe000, 0xffff, mirror_ram_w				},	// Mirrored RAM (agallet)
MEMORY_END

static PORT_READ_START( sailormn_sound_readport )
	{ 0x20, 0x20, soundflags_r				},	// Communication
	{ 0x30, 0x30, soundlatch_lo_r			},	// From Main CPU
	{ 0x40, 0x40, soundlatch_hi_r			},	//
	{ 0x51, 0x51, YM2151_status_port_0_r	},	// YM2151
	{ 0x60, 0x60, OKIM6295_status_0_r		},	// M6295 #0
	{ 0x80, 0x80, OKIM6295_status_1_r		},	// M6295 #1
PORT_END

static PORT_WRITE_START( sailormn_sound_writeport )
	{ 0x00, 0x00, sailormn_rombank_w		},	// Rom Bank
	{ 0x10, 0x10, IOWP_NOP			},	// To Main CPU
	{ 0x50, 0x50, YM2151_register_port_0_w	},	// YM2151
	{ 0x51, 0x51, YM2151_data_port_0_w		},	//
	{ 0x60, 0x60, OKIM6295_data_0_w			},	// M6295 #0
	{ 0x70, 0x70, sailormn_okibank0_w		},	// Samples Bank #0
	{ 0x80, 0x80, OKIM6295_data_1_w			},	// M6295 #1
	{ 0xc0, 0xc0, sailormn_okibank1_w		},	// Samples Bank #1
PORT_END

static void irqhandler(int irq)
{
	cpu_set_irq_line(0,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_intf_4MHz =
{
	1,
	16000000/4, /* ? */
	{ YM3012_VOL(25,MIXER_PAN_LEFT,25,MIXER_PAN_RIGHT) },
	{ irqhandler }, /* irq handler */
	{ 0 } /* port_write */
};

static struct OKIM6295interface okim6295_intf_16kHz_16kHz =
{
	2,
	{ 16000, 16000 },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 70, 70 }
};

static void AG_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	if (cmda > 32)	// SFX
	{
		cmd_latch = 0x5081 + (cmda - 31);
	}

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( airgallet )
	MDRV_NAME("Air Gallet")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_SEND( AG_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(sailormn_sound_readmem,sailormn_sound_writemem)
	MDRV_CPU_PORTS(sailormn_sound_readport,sailormn_sound_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_intf_4MHz)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_intf_16kHz_16kHz)
M1_BOARD_END
