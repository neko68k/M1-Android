// Tecmo System
// Z80 + OPL3 (YMF-262) + YMZ280B + MSM-6295 (yikes)

#include "m1snd.h"

static void YM3812IRQ(int irq);

static int cmd_latch, bnkofs;

static void YM3812IRQ(int irq)
{
//	printf("3812 IRQ: %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static struct YMF262interface ymf262_interface =
{
	1,					/* 1 chip */
	14318180,			/* X1 ? */
	{ YAC512_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },	/* channels A and B */
	{ YAC512_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },	/* channels C and D */
	{ YM3812IRQ },		/* irq */
};

static struct YMZ280Binterface ymz280b_interface =
{
	1,
	{ 16934400 },
	{ RGN_SAMP1 },
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
};

static struct OKIM6295interface okim6295_interface =
{
	1,  /* 1 chip */
	{ 16000 },
	{ RGN_SAMP2 },
	{ 50 }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( deroon_bankswitch_w )
{
	cpu_setbank( 1, memory_region(REGION_CPU1) + (data & 0x0f) * 0x4000 );
}

static WRITE_HANDLER( deroon_okibank_w )
{
	int bank;

	bank = (data & 0x10) ? 0 : 0x40000;

	OKIM6295_set_bank_base(0, 0x00000);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xe000, 0xf7ff, MRA_RAM },

MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xf7ff, MWA_RAM },
MEMORY_END



static PORT_READ_START( readport )
	{ 0x00, 0x00, YMF262_status_0_r },
	{ 0x40, 0x40, latch_r },
	//{ 0x60, 0x60, YMZ280B_status_0_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, YMF262_register_A_0_w },
	{ 0x01, 0x01, YMF262_data_A_0_w },
	{ 0x02, 0x02, YMF262_register_B_0_w },
	{ 0x03, 0x03, YMF262_data_B_0_w },

	{ 0x10, 0x10, OKIM6295_data_0_w },

	{ 0x20, 0x20, deroon_okibank_w },

	{ 0x30, 0x30, deroon_bankswitch_w },

	//{ 0x50, 0x50, to_main_cpu_latch_w },
	{ 0x50, 0x50, IOWP_NOP },

//	{ 0x60, 0x60, YMZ280B_register_0_w },
//	{ 0x61, 0x61, YMZ280B_data_0_w },
	{ 0x60, 0x61, IOWP_NOP },
PORT_END

static void TSYS_Init(long srate)
{
	bnkofs = 0x8000;
}

static void TSYS_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( tecmosys )
	MDRV_NAME("Tecmo System")
	MDRV_HWDESC("Z80, YMF-262, YMZ280B, MSM-6295")
	MDRV_INIT( TSYS_Init )
	MDRV_SEND( TSYS_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)	// part is -8, music slows down if you make it 4 MHz...
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(readport,writeport)

	MDRV_SOUND_ADD(YMF262, &ymf262_interface)
	MDRV_SOUND_ADD(YMZ280B, &ymz280b_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

