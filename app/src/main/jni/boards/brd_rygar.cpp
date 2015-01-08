/* Various Tecmo games */

#include "m1snd.h"

static void Rygar_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static int adpcm_pos,adpcm_end;

static WRITE_HANDLER( tecmo_adpcm_start_w )
{
	adpcm_pos = data << 8;
	MSM5205_reset_w(0,0);
}
static WRITE_HANDLER( tecmo_adpcm_end_w )
{
	adpcm_end = (data + 1) << 8;
}
static WRITE_HANDLER( tecmo_adpcm_vol_w )
{
	MSM5205_set_volume(0,(data & 0x0f) * 100 / 15);
}
static void tecmo_adpcm_int(int num)
{
	static int adpcm_data = -1;

	if (adpcm_pos >= adpcm_end ||
				adpcm_pos >= memory_region_length(REGION_SOUND1))
		MSM5205_reset_w(0,1);
	else if (adpcm_data != -1)
	{
		MSM5205_data_w(0,adpcm_data & 0x0f);
		adpcm_data = -1;
	}
	else
	{
		unsigned char *ROM = memory_region(REGION_SOUND1);

		adpcm_data = ROM[adpcm_pos++];
		MSM5205_data_w(0,adpcm_data >> 4);
	}
}

static MEMORY_READ_START( rygar_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xc000, 0xc000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( rygar_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, YM3812_control_port_0_w },
	{ 0x8001, 0x8001, YM3812_write_port_0_w },
	{ 0xc000, 0xc000, tecmo_adpcm_start_w },
	{ 0xd000, 0xd000, tecmo_adpcm_end_w },
	{ 0xe000, 0xe000, tecmo_adpcm_vol_w },
	{ 0xf000, 0xf000, MWA_NOP },	/* NMI acknowledge */
MEMORY_END

static struct YM3526interface ym3812_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ 90 },		/* volume */
	{ YM_IRQHandler }
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	384000,				/* 384KHz             */
	{ tecmo_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B },	/* 8KHz               */
	{ 50 }				/* volume */
};

M1_BOARD_START( rygar )
	MDRV_NAME("Rygar")
	MDRV_HWDESC("Z80, YM3812, MSM-5205")
	MDRV_SEND( Rygar_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(rygar_sound_readmem,rygar_sound_writemem)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(MSM5205, &msm5205_interface)
M1_BOARD_END

static MEMORY_READ_START( tecmo_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xc000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( tecmo_sound_writemem )
	{ 0x2000, 0x207f, MWA_RAM },	/* Silkworm set #2 has a custom CPU which */
									/* writes code to this area */
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xc000, 0xc000, tecmo_adpcm_start_w },
	{ 0xc400, 0xc400, tecmo_adpcm_end_w },
	{ 0xc800, 0xc800, tecmo_adpcm_vol_w },
	{ 0xcc00, 0xcc00, MWA_NOP },	/* NMI acknowledge */
MEMORY_END

M1_BOARD_START( tecmo )
	MDRV_NAME("Tecmo 8-bit")
	MDRV_HWDESC("Z80, YM3812, MSM-5205")
	MDRV_SEND( Rygar_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(tecmo_sound_readmem,tecmo_sound_writemem)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(MSM5205, &msm5205_interface)
M1_BOARD_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, YM2608_status_port_0_A_r },
	{ 0xf802, 0xf802, YM2608_status_port_0_B_r },
	{ 0xfc00, 0xfc00, MRA_NOP }, /* ??? adpcm ??? */
	{ 0xfc10, 0xfc10, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2608_control_port_0_A_w },
	{ 0xf801, 0xf801, YM2608_data_port_0_A_w },
	{ 0xf802, 0xf802, YM2608_control_port_0_B_w },
	{ 0xf803, 0xf803, YM2608_data_port_0_B_w },
MEMORY_END

static struct YM2608interface ym2608_interface =
{
	1,
	8000000,
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQHandler },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

M1_BOARD_START( wc90 )
	MDRV_NAME("World Cup '90")
	MDRV_HWDESC("Z80, YM2608")
	MDRV_SEND( Rygar_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2608, &ym2608_interface)
M1_BOARD_END

static void YM_IRQHandler(int irq)
{
//	printf("YM IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);

}

static void Rygar_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
