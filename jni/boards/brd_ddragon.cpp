/* Technos Double Dragon */
/* HD6309 + YM2151 + MSM-5205 */

/* Xain d'Sleena */
/* HD6309 + YM2203 x 2 */

#include "m1snd.h"

static int cmd_latch;
static int adpcm_pos[2],adpcm_end[2],adpcm_idle[2];

static void Dragon_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	hd6309_set_irq_line(HD6309_IRQ_LINE, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	hd6309_set_irq_line(HD6309_IRQ_LINE, CLEAR_LINE);
	return cmd_latch;
}

static void irq_handler(int irq)
{
	hd6309_set_irq_line(HD6309_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( dd_adpcm_w )
{
	int chip = offset & 1;

	switch (offset/2)
	{
		case 3:
			adpcm_idle[chip] = 1;
			MSM5205_reset_w(chip,1);
			break;

		case 2:
			adpcm_pos[chip] = (data & 0x7f) * 0x200;
			break;

		case 1:
			adpcm_end[chip] = (data & 0x7f) * 0x200;
			break;

		case 0:
			adpcm_idle[chip] = 0;
			MSM5205_reset_w(chip,0);
			break;
	}
}

static void dd_adpcm_int(int chip)
{
	static int adpcm_data[2] = { -1, -1 };

	if (adpcm_pos[chip] >= adpcm_end[chip] || adpcm_pos[chip] >= 0x10000)
	{
		adpcm_idle[chip] = 1;
		MSM5205_reset_w(chip,1);
	}
	else if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip,adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
	}
	else
	{
		unsigned char *ROM = memory_region(REGION_SOUND1) + 0x10000 * chip;

		adpcm_data[chip] = ROM[adpcm_pos[chip]++];
		MSM5205_data_w(chip,adpcm_data[chip] >> 4);
	}
}

static READ_HANDLER( dd_adpcm_status_r )
{
	return adpcm_idle[0] + (adpcm_idle[1] << 1);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct MSM5205interface msm5205_interface =
{
	2,					/* 2 chips             */
	384000,				/* 384KHz             */
	{ dd_adpcm_int, dd_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B, MSM5205_S48_4B },	/* 8kHz */
	{ 40, 40 }				/* volume */
};

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, latch_r },
	{ 0x1800, 0x1800, dd_adpcm_status_r },
	{ 0x2800, 0x2801, YM2151_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x2800, 0x2800, YM2151_register_port_0_w },
	{ 0x2801, 0x2801, YM2151_data_port_0_w },
	{ 0x3800, 0x3807, dd_adpcm_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

M1_BOARD_START( ddragon )
	MDRV_NAME("Double Dragon")
	MDRV_HWDESC("HD6309, YM2151, MSM-5205(x2)")
	MDRV_SEND( Dragon_SendCmd )

	MDRV_CPU_ADD(HD6309, 3579545)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, &msm5205_interface)
M1_BOARD_END


/*
	Xain d'Sleena
*/
static void irqhandler(int irq)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,
	3000000,
	{ YM2203_VOL(40,50), YM2203_VOL(40,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static MEMORY_READ_START( xain_sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, latch_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( xain_sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2800, 0x2800, YM2203_control_port_0_w },
	{ 0x2801, 0x2801, YM2203_write_port_0_w },
	{ 0x3000, 0x3000, YM2203_control_port_1_w },
	{ 0x3001, 0x3001, YM2203_write_port_1_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

M1_BOARD_START( xsleena )
	MDRV_NAME("Xain d'Sleena")
	MDRV_HWDESC("HD6309, YM22203(x2)")
	MDRV_SEND( Dragon_SendCmd )

	MDRV_CPU_ADD(HD6309, 1500000)
	MDRV_CPU_MEMORY(xain_sound_readmem, xain_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END