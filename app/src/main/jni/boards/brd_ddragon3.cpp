/* Technos Double Dragon 2 + DDragon 3 + Combatribes + WWF Super Stars + WWF Wrestlefest + Block Out */
/* Ddragon3, Wrestlefest, and Shadow Force share a sound board (Shadow Force has more workram but is otherwise identical) */
/* DDragon2, Combatribes, China Gate, and Block Out share a slightly simpler board (no ADPCM banking) */
/* Mug Smashers rips off ddragon2's sound hardware *and* sound software */
/* Z80 + YM2151 + MSM6295 */

#include "m1snd.h"

#define Z80_CLOCK (3579545)
#define YM_CLOCK  (3579545)
#define OKI_CLOCK (8500)
#define OKI_CLOCK_DD2 (8000)

static void DDg2_Init(long srate);
static void DDg3_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* Guess */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ YM_IRQHandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,	       	/* 1 chip */
	{ 8500 },      	/* frequency (Hz) */
	{ RGN_SAMP1 },	/* memory region */
	{ 47 }
};

static struct OKIM6295interface dd2_okim6295_interface =
{
	1,              /* 1 chip */
	{ 8000 },           /* frequency (Hz) */
	{ RGN_SAMP1 },	/* memory region */
	{ 15 }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( oki_bankswitch_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

M1_BOARD_START( ddragon2 )
	MDRV_NAME( "Double Dragon 2" )
	MDRV_HWDESC( "Z80, YM2151, MSM-6295" )
	MDRV_DELAYS( 300, 100 )
	MDRV_INIT( DDg2_Init )
	MDRV_SEND( DDg3_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem, writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &dd2_okim6295_interface)
M1_BOARD_END

static MEMORY_READ_START( ddg3_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc801, 0xc801, YM2151_status_port_0_r },
	{ 0xd800, 0xd800, OKIM6295_status_0_r },
	{ 0xe000, 0xe000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( ddg3_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, YM2151_register_port_0_w },
	{ 0xc801, 0xc801, YM2151_data_port_0_w },
	{ 0xd800, 0xd800, OKIM6295_data_0_w },
	{ 0xe800, 0xe800, oki_bankswitch_w },
MEMORY_END

M1_BOARD_START( ddragon3 )
	MDRV_NAME( "Double Dragon 3" )
	MDRV_HWDESC( "Z80, YM2151, MSM-6295" )
	MDRV_DELAYS( 300, 100 )
	MDRV_INIT( DDg2_Init )
	MDRV_SEND( DDg3_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(ddg3_readmem, ddg3_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static void YM_IRQHandler(int irq)
{
//	printf("YM IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);

}

static void DDg2_Init(long srate)
{
	// combatribes has it's sample rom a bit misordered
	if (Machine->refcon == 1)
	{
		char *ROM;

		ROM = (char *)rom_getregion(RGN_SAMP1);

		memcpy(ROM, ROM + 0x40000, 0x20000);
	}
}

static void DDg3_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
