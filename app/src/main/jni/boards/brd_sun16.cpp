/* SunA 16-bit hardware */
/* One master Z80 driving a YM2151,
   two slaves driving 4-bit DACs */

#include "m1snd.h"

#define SA16_Z80_CLOCK (3579545)	// master Z80 is at the YM2151 clock
#define SA16_SUB_CLOCK (5000000)	// slaves are faster

static void SA16_Init(long srate);
static void SA16_SendCmd(int cmda, int cmdb);

static int cmd_latch, sub_latch, sub2_latch;

static struct YM2151interface bssoccer_ym2151_interface =
{
	1,
	3579545,	/* ? */
	{ YM3012_VOL(20,MIXER_PAN_LEFT, 20,MIXER_PAN_RIGHT) },
	{ 0 },		/* irq handler */
	{ 0 }		/* port write handler */
};

static struct DACinterface bssoccer_dac_interface =
{
	4,
	{	MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT),
		MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT)	}
};

static struct YM2151interface uballoon_ym2151_interface =
{
	1,
	3579545,	/* ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 },		/* irq handler */
	{ 0 }		/* port write handler */
};

static struct DACinterface uballoon_dac_interface =
{
	2,
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) }
};

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static READ_HANDLER( latch2_r )
{
	return sub_latch;
}

static READ_HANDLER( latch3_r )
{
	return sub2_latch;
}

static WRITE_HANDLER( latch2_w )
{
	sub_latch = data;
}

static WRITE_HANDLER( latch3_w )
{
	sub2_latch = data;
}

static MEMORY_READ_START( uballoon_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM					},	// ROM
	{ 0xf000, 0xf7ff, MRA_RAM					},	// RAM
	{ 0xf801, 0xf801, YM2151_status_port_0_r	},	// YM2151
	{ 0xfc00, 0xfc00, latch_r				},	// From Main CPU
MEMORY_END

static MEMORY_WRITE_START( uballoon_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM					},	// ROM
	{ 0xf000, 0xf7ff, MWA_RAM					},	// RAM
	{ 0xf800, 0xf800, YM2151_register_port_0_w	},	// YM2151
	{ 0xf801, 0xf801, YM2151_data_port_0_w		},	//
	{ 0xfc00, 0xfc00, latch2_w				},	// To PCM Z80
MEMORY_END

static WRITE_HANDLER( uballoon_pcm_1_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bank = data & 1;
	if (bank & ~1)	logerror((char *)"CPU#2 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	cpu_setbank(1, &RAM[bank * 0x10000 + 0x400]);
}

/* 2 DACs per CPU - 4 bits per sample */

static WRITE_HANDLER( bssoccer_DAC_1_w )
{
	DAC_data_w( 0 + (offset & 1), (data & 0xf) * 0x11 );
}

static WRITE_HANDLER( bssoccer_DAC_2_w )
{
	DAC_data_w( 2 + (offset & 1), (data & 0xf) * 0x11 );
}

/* Memory maps: Yes, *no* RAM */

static MEMORY_READ_START( uballoon_pcm_1_readmem )
	{ 0x0000, 0x03ff, MRA_ROM			},	// ROM
	{ 0x0400, 0xffff, MRA_BANK1 		},	// Banked ROM
MEMORY_END
static MEMORY_WRITE_START( uballoon_pcm_1_writemem )
	{ 0x0000, 0xffff, MWA_ROM			},	// ROM
MEMORY_END


static PORT_READ_START( uballoon_pcm_1_readport )
	{ 0x00, 0x00, latch2_r 				},	// From The Sound Z80
PORT_END
static PORT_WRITE_START( uballoon_pcm_1_writeport )
	{ 0x00, 0x01, bssoccer_DAC_1_w				},	// 2 x DAC
	{ 0x03, 0x03, uballoon_pcm_1_bankswitch_w	},	// Rom Bank
PORT_END

M1_BOARD_START( suna16 )
	MDRV_NAME("SunA16 (type 1)")
	MDRV_HWDESC("Z80(x2), YM2151, DAC(x2)")
	MDRV_DELAYS( 800, 100 )
	MDRV_INIT( SA16_Init )
	MDRV_SEND( SA16_SendCmd )

	MDRV_CPU_ADD(Z80C, SA16_Z80_CLOCK)
	MDRV_CPU_MEMORY(uballoon_sound_readmem,uballoon_sound_writemem)

	MDRV_CPU_ADD(Z80C, SA16_SUB_CLOCK)
	MDRV_CPU_MEMORY(uballoon_pcm_1_readmem,uballoon_pcm_1_writemem)
	MDRV_CPU_PORTS(uballoon_pcm_1_readport,uballoon_pcm_1_writeport)

	MDRV_SOUND_ADD(YM2151, &uballoon_ym2151_interface)
	MDRV_SOUND_ADD(DAC, &uballoon_dac_interface)
M1_BOARD_END

static WRITE_HANDLER( bssoccer_pcm_1_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bank = data & 7;
	cpu_setbank(1, &RAM[bank * 0x10000 + 0x1000]);
}

static WRITE_HANDLER( bssoccer_pcm_2_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int bank = data & 7;
	cpu_setbank(2, &RAM[bank * 0x10000 + 0x1000]);
}

static MEMORY_READ_START( bssoccer_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	// ROM
	{ 0xf000, 0xf7ff, MRA_RAM					},	// RAM
	{ 0xf801, 0xf801, YM2151_status_port_0_r	},	// YM2151
	{ 0xfc00, 0xfc00, latch_r				},	// From Main CPU
MEMORY_END

static MEMORY_WRITE_START( bssoccer_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	// ROM
	{ 0xf000, 0xf7ff, MWA_RAM					},	// RAM
	{ 0xf800, 0xf800, YM2151_register_port_0_w	},	// YM2151
	{ 0xf801, 0xf801, YM2151_data_port_0_w		},	//
	{ 0xfd00, 0xfd00, latch2_w 			},	// To PCM Z80 #1
	{ 0xfe00, 0xfe00, latch3_w 			},	// To PCM Z80 #2
MEMORY_END

static MEMORY_READ_START( bssoccer_pcm_1_readmem )
	{ 0x0000, 0x0fff, MRA_ROM			},	// ROM
	{ 0x1000, 0xffff, MRA_BANK1 		},	// Banked ROM
MEMORY_END
static MEMORY_WRITE_START( bssoccer_pcm_1_writemem )
	{ 0x0000, 0xffff, MWA_ROM			},	// ROM
MEMORY_END


static MEMORY_READ_START( bssoccer_pcm_2_readmem )
	{ 0x0000, 0x0fff, MRA_ROM			},	// ROM
	{ 0x1000, 0xffff, MRA_BANK2 		},	// Banked ROM
MEMORY_END
static MEMORY_WRITE_START( bssoccer_pcm_2_writemem )
	{ 0x0000, 0xffff, MWA_ROM			},	// ROM
MEMORY_END


static PORT_READ_START( bssoccer_pcm_1_readport )
	{ 0x00, 0x00, latch2_r 				},	// From The Sound Z80
PORT_END
static PORT_WRITE_START( bssoccer_pcm_1_writeport )
	{ 0x00, 0x01, bssoccer_DAC_1_w				},	// 2 x DAC
	{ 0x03, 0x03, bssoccer_pcm_1_bankswitch_w	},	// Rom Bank
PORT_END

static PORT_READ_START( bssoccer_pcm_2_readport )
	{ 0x00, 0x00, latch3_r 				},	// From The Sound Z80
PORT_END
static PORT_WRITE_START( bssoccer_pcm_2_writeport )
	{ 0x00, 0x01, bssoccer_DAC_2_w				},	// 2 x DAC
	{ 0x03, 0x03, bssoccer_pcm_2_bankswitch_w	},	// Rom Bank
PORT_END

M1_BOARD_START( suna16b )
	MDRV_NAME("SunA16 (type 2)")
	MDRV_HWDESC("Z80(x3), YM2151, DAC(x4)")
	MDRV_DELAYS( 800, 100 )
	MDRV_INIT( SA16_Init )
	MDRV_SEND( SA16_SendCmd )

	MDRV_CPU_ADD(Z80C, SA16_Z80_CLOCK)
	MDRV_CPU_MEMORY(bssoccer_sound_readmem,bssoccer_sound_writemem)

	MDRV_CPU_ADD(Z80C, SA16_SUB_CLOCK)
	MDRV_CPU_MEMORY(bssoccer_pcm_1_readmem,bssoccer_pcm_1_writemem)
	MDRV_CPU_PORTS(bssoccer_pcm_1_readport,bssoccer_pcm_1_writeport)

	MDRV_CPU_ADD(Z80C, SA16_SUB_CLOCK)
	MDRV_CPU_MEMORY(bssoccer_pcm_2_readmem,bssoccer_pcm_2_writemem)
	MDRV_CPU_PORTS(bssoccer_pcm_2_readport,bssoccer_pcm_2_writeport)

	MDRV_SOUND_ADD(YM2151, &bssoccer_ym2151_interface)
	MDRV_SOUND_ADD(DAC, &bssoccer_dac_interface)
M1_BOARD_END

static void SA16_Init(long srate)
{
	cmd_latch = 210;
}

static void SA16_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
