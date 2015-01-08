// Strikers 1945
// Gunbird

#include "m1snd.h"

static void S19_SendCmd(int cmda, int cmdb);
static void YM3812IRQ(int irq);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct YMF278B_interface opl4parm =
{
	1,
	{ YMF278B_STD_CLOCK },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) },
	{ YM3812IRQ }
};
struct YM2610interface ym2610_interface =
{
	1,
	8000000,	/* ? */
	{ MIXERG(30,MIXER_GAIN_4x,MIXER_PAN_CENTER) },
	{ 0 },	/* A_r */
	{ 0 },	/* B_r */
	{ 0 },	/* A_w */
	{ 0 },	/* B_w */
	{ YM3812IRQ },	/* irq */
	{ RGN_SAMP1 },	/* delta_t */
	{ RGN_SAMP2 },	/* adpcm */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

WRITE_HANDLER( gunbird_sound_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = (data >> 4) & 3;

	/* The banked rom is seen at 8200-ffff, so the last 0x200 bytes
	   of the rom not reachable. */

	cpu_setbank(1, &RAM[bank * 0x8000 + 0x200]);
}



static MEMORY_READ_START( gunbird_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM		},	// ROM
	{ 0x8000, 0x81ff, MRA_RAM		},	// RAM
	{ 0x8200, 0xffff, MRA_BANK1		},	// Banked ROM
MEMORY_END

static MEMORY_WRITE_START( gunbird_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM		},	// ROM
	{ 0x8000, 0x81ff, MWA_RAM		},	// RAM
	{ 0x8200, 0xffff, MWA_ROM		},	// Banked ROM
MEMORY_END


static PORT_READ_START( gunbird_sound_readport )
	{ 0x04, 0x04, YM2610_status_port_0_A_r		},
	{ 0x06, 0x06, YM2610_status_port_0_B_r		},
	{ 0x08, 0x08, latch_r					},
PORT_END

static PORT_WRITE_START( gunbird_sound_writeport )
	{ 0x00, 0x00, gunbird_sound_bankswitch_w	},
	{ 0x04, 0x04, YM2610_control_port_0_A_w		},
	{ 0x05, 0x05, YM2610_data_port_0_A_w		},
	{ 0x06, 0x06, YM2610_control_port_0_B_w		},
	{ 0x07, 0x07, YM2610_data_port_0_B_w		},
	{ 0x0c, 0x0c, IOWP_NOP			},
PORT_END

static PORT_READ_START( s1945_sound_readport )
	{ 0x08, 0x08, YMF278B_status_port_0_r		},
	{ 0x10, 0x10, latch_r					},
PORT_END

static PORT_WRITE_START( s1945_sound_writeport )
	{ 0x00, 0x00, gunbird_sound_bankswitch_w	},
	{ 0x02, 0x03, IOWP_NOP						},
	{ 0x08, 0x08, YMF278B_control_port_0_A_w	},
	{ 0x09, 0x09, YMF278B_data_port_0_A_w		},
	{ 0x0a, 0x0a, YMF278B_control_port_0_B_w	},
	{ 0x0b, 0x0b, YMF278B_data_port_0_B_w		},
	{ 0x0c, 0x0c, YMF278B_control_port_0_C_w	},
	{ 0x0d, 0x0d, YMF278B_data_port_0_C_w		},
	{ 0x18, 0x18, IOWP_NOP			},
PORT_END

M1_BOARD_START( s1945 )
	MDRV_NAME("Strikers 1945")
	MDRV_HWDESC("Z80, YMF278B (OPL4)")
	MDRV_DELAYS(1500, 100)
	MDRV_SEND( S19_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(gunbird_sound_readmem,gunbird_sound_writemem)
	MDRV_CPU_PORTS(s1945_sound_readport,s1945_sound_writeport)

	MDRV_SOUND_ADD(YMF278B, &opl4parm)
M1_BOARD_END

M1_BOARD_START( gunbird )
	MDRV_NAME("Gunbird")
	MDRV_HWDESC("Z80, YM2610")
	MDRV_DELAYS(1500, 100)
	MDRV_SEND( S19_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(gunbird_sound_readmem,gunbird_sound_writemem)
	MDRV_CPU_PORTS(gunbird_sound_readport,gunbird_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &ym2610_interface)
M1_BOARD_END

/* IRQ Handler */
static void YM3812IRQ(int irq)
{
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void S19_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
