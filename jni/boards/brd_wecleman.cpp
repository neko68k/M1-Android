// WEC Le Mans (Konami)

#include "m1snd.h"

static void WL_SendCmd(int cmda, int cmdb);
static WRITE_HANDLER( k007232_extvolume_w );

static int cmd_latch;
static int multiply_reg[2];

static struct YM2151interface ym2151_interface =
{
	1,
	3579545,        /* same as sound cpu */
	//{ 80 },
	{ YM3012_VOL(85,MIXER_PAN_LEFT,85,MIXER_PAN_RIGHT) }, //AT
	{ 0  }
};


static struct K007232_interface wecleman_k007232_interface =
{
	1,
	3579545,	/* clock */
	{ REGION_SOUND1 },      /* but the 2 channels use different ROMs !*/
	{ K007232_VOL( 20,MIXER_PAN_LEFT, 20,MIXER_PAN_RIGHT ) },
	{0}
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( wecleman_K00723216_bank_w )
{
	K007232_set_bank( 0, 0, data&1 );
}

/* Protection - an external multiplyer connected to the sound CPU */
static READ_HANDLER( multiply_r )
{
	return (multiply_reg[0] * multiply_reg[1]) & 0xFF;
}
static WRITE_HANDLER( multiply_w )
{
	multiply_reg[offset] = data;
}

static MEMORY_READ_START( wecleman_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM                               },      // ROM
	{ 0x8000, 0x83ff, MRA_RAM                               },      // RAM
	{ 0x9000, 0x9000, multiply_r                            },      // Protection
	{ 0xa000, 0xa000, latch_r                          },      // From main CPU
	{ 0xb000, 0xb00d, K007232_read_port_0_r                 },      // K007232 (Reading offset 5/b triggers the sample)
	{ 0xc001, 0xc001, YM2151_status_port_0_r                },      // YM2151
MEMORY_END

static MEMORY_WRITE_START( wecleman_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM                               },      // ROM
	{ 0x8000, 0x83ff, MWA_RAM                               },      // RAM
	//{ 0x8500, 0x8500, MWA_NOP                               },      // incresed with speed (global volume)?
	{ 0x9000, 0x9001, multiply_w                            },      // Protection
	//{ 0x9006, 0x9006, MWA_NOP                               },      // ?
	{ 0xb000, 0xb00d, K007232_write_port_0_w                },      // K007232
	{ 0xc000, 0xc000, YM2151_register_port_0_w              },      // YM2151
	{ 0xc001, 0xc001, YM2151_data_port_0_w                  },
	{ 0xf000, 0xf000, wecleman_K00723216_bank_w             },      // Samples banking
MEMORY_END

M1_BOARD_START( weclemans )
	MDRV_NAME("WEC Le Mans")
	MDRV_HWDESC("Z80, YM2151, K007232")
	MDRV_SEND( WL_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(wecleman_sound_readmem,wecleman_sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &wecleman_k007232_interface)
M1_BOARD_END

static MEMORY_READ_START( chqflag_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM */
	{ 0xa000, 0xa00d, K007232_read_port_0_r },	/* 007232 (chip 1) */
	{ 0xb000, 0xb00d, K007232_read_port_1_r },	/* 007232 (chip 2) */
	{ 0xc001, 0xc001, YM2151_status_port_0_r },	/* YM2151 */
	{ 0xd000, 0xd000, latch_r },			/* soundlatch_r */
	//{ 0xe000, 0xe000, MRA_NOP },				/* ??? */
MEMORY_END

static WRITE_HANDLER( k007232_bankswitch_w )
{
	int bank_A, bank_B;

	/* banks # for the 007232 (chip 1) */
	bank_A = ((data >> 4) & 0x03);
	bank_B = ((data >> 6) & 0x03);
	K007232_set_bank( 0, bank_A, bank_B );

	/* banks # for the 007232 (chip 2) */
	bank_A = ((data >> 0) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 1, bank_A, bank_B );
}

static MEMORY_WRITE_START( chqflag_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM */
	{ 0x9000, 0x9000, k007232_bankswitch_w },		/* 007232 bankswitch */
	{ 0xa000, 0xa00d, K007232_write_port_0_w },		/* 007232 (chip 1) */
	{ 0xa01c, 0xa01c, k007232_extvolume_w },/* extra volume, goes to the 007232 w/ A11 */
											/* selecting a different latch for the external port */
	{ 0xb000, 0xb00d, K007232_write_port_1_w },		/* 007232 (chip 2) */
	{ 0xc000, 0xc000, YM2151_register_port_0_w },	/* YM2151 */
	{ 0xc001, 0xc001, YM2151_data_port_0_w },		/* YM2151 */
	{ 0xf000, 0xf000, MWA_NOP },					/* ??? */
MEMORY_END

static void chqflag_ym2151_irq_w(int data)
{
	if (data)
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	else
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
}


static struct YM2151interface ym2151_interface_chq =
{
	1,
	3579545,	/* 3.579545 MHz? */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ chqflag_ym2151_irq_w },
	{ 0 }
};

static void volume_callback0(int v)
{
	K007232_set_volume(0,0,(v & 0x0f)*0x11,0);
	K007232_set_volume(0,1,0,(v >> 4)*0x11);
}

static WRITE_HANDLER( k007232_extvolume_w )
{
	K007232_set_volume(1,1,(data & 0x0f)*0x11/2,(data >> 4)*0x11/2);
}

static void volume_callback1(int v)
{
	K007232_set_volume(1,0,(v & 0x0f)*0x11/2,(v >> 4)*0x11/2);
}

static struct K007232_interface k007232_interface_chq =
{
	2,															/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2 },							/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER),		/* volume */
		K007232_VOL(20,MIXER_PAN_LEFT,20,MIXER_PAN_RIGHT) },
	{ volume_callback0,  volume_callback1 }						/* external port callback */
};

M1_BOARD_START( chqflag )
	MDRV_NAME("Chequered Flag")
	MDRV_HWDESC("Z80, YM2151, K007232(x2)")
	MDRV_SEND( WL_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(chqflag_readmem_sound,chqflag_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface_chq)
	MDRV_SOUND_ADD(K007232, &k007232_interface_chq)
M1_BOARD_END

static void WL_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
 	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
