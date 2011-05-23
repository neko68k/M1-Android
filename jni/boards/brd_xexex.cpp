/* Konami XEXEX and relatives sound */

#include "m1snd.h"

#define Z80_CLOCK (8000000)
#define YM_CLOCK (4000000)
#define K539_CLOCK (48000)

#define PGX_Z80_CLOCK (8000000)

static void PGX_Init(long srate);
static void XEX_SendCmd(int cmda, int cmdb);
void pgx_timer(void);

static int cmd_latch;
static void ym_set_mixing(double left, double right);

static READ_HANDLER( latch2_r )
{
  	cpu_set_irq_line(0, 0, CLEAR_LINE);

	if (Machine->refcon == 1)
	{
		return cmd_latch&0xff;
	}

	if (Machine->refcon == 2)	// Gaiapolis
	{
		if (cmd_latch < 0x100)
		{
			if (!offset)
			{
				return 0xf9;
			}

			return cmd_latch&0xff;
		}
		else
		{
			if (!offset)
			{
				return cmd_latch&0xff;
			}

			if (cmd_latch >= 0xf0)
			{
				return 1;
			}

			return 0;
		}
	}
	else
	{
		if (!offset)
		{
			return cmd_latch;
		}

		if (Machine->refcon == 3)
		{
			return 0x80;
		}
	}

	if (cmd_latch >= 0xf0)
	{
		return 1;
	}
	return 0;
}

static READ_HANDLER( latch_r )
{
	int ret = latch2_r(offset);

//	printf("read command @ %x = %x\n", offset, ret);

	return ret;
}

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ 0 }
};

static struct YM2151interface xmen_ym2151_interface =
{
	1,
	4000000,
	{ YM3012_VOL(15,MIXER_PAN_CENTER,15,MIXER_PAN_CENTER) },
	{ 0 }
};

static struct K054539interface k054539_interface =
{
	2,
	48000,
	{ RGN_SAMP1, RGN_SAMP1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL },
	{ pgx_timer }
};

static struct K054539interface xex_k054539_interface =
{
	1,
	48000,
	{ RGN_SAMP1 },
	{ { 50, 50 } },
	{ ym_set_mixing }
};

static struct K054539interface gij_k054539_interface =
{
	1,
	48000,
	{ RGN_SAMP1 },
	{ { 100, 100 } },
	{ NULL },
	{ pgx_timer }
};

static WRITE_HANDLER( sound_bankswitch_w )
{
	int cur_sound_region = (data & 0xf);
//	printf("bankswitch %x\n", data);
	cpu_setbank(2, memory_region(REGION_CPU1) + cur_sound_region*0x4000);
}

static MEMORY_READ_START( xex_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xe801, 0xe801, YM2151_status_port_0_r },	// mirror for x-men
	{ 0xec01, 0xec01, YM2151_status_port_0_r },
	{ 0xf002, 0xf003, latch_r },
MEMORY_END

static MEMORY_WRITE_START( xex_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe800, 0xe800, YM2151_register_port_0_w },	// mirror for x-men
	{ 0xe801, 0xe801, YM2151_data_port_0_w },
	{ 0xec00, 0xec00, YM2151_register_port_0_w },
	{ 0xec01, 0xec01, YM2151_data_port_0_w },
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf800, 0xf800, sound_bankswitch_w },
MEMORY_END

M1_BOARD_START( xexex )
	MDRV_NAME( "Xexex" )
	MDRV_HWDESC( "Z80, YM2151, K054539" )
	MDRV_DELAYS( 60, 10 )
	MDRV_INIT( PGX_Init )
	MDRV_SEND( XEX_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(xex_sound_readmem, xex_sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K054539, &xex_k054539_interface)
M1_BOARD_END

M1_BOARD_START( xmen )
	MDRV_NAME( "X-Men" )
	MDRV_HWDESC( "Z80, YM2151, K054539" )
	MDRV_DELAYS( 60, 10 )
	MDRV_INIT( PGX_Init )
	MDRV_SEND( XEX_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(xex_sound_readmem, xex_sound_writemem)

	MDRV_SOUND_ADD(YM2151, &xmen_ym2151_interface)
	MDRV_SOUND_ADD(K054539, &xex_k054539_interface)
M1_BOARD_END

static MEMORY_READ_START( pgx_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xe230, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe62f, K054539_1_r },
	{ 0xe630, 0xe7ff, MRA_RAM },
	{ 0xf002, 0xf003, latch_r },
MEMORY_END

static MEMORY_WRITE_START( pgx_sound_writemem )
	{ 0x0000, 0xbfff, MWA_NOP },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe230, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe62f, K054539_1_w },
	{ 0xe630, 0xe7ff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf800, 0xf800, sound_bankswitch_w },
	{ 0xfff1, 0xfff3, MWA_NOP },
MEMORY_END

M1_BOARD_START( pregx )
	MDRV_NAME( "Mystic Warrior" )
	MDRV_HWDESC( "Z80, K054539(x2)" )
	MDRV_DELAYS( 60, 10 )
	MDRV_INIT( PGX_Init )
	MDRV_SEND( XEX_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(pgx_sound_readmem, pgx_sound_writemem)

	MDRV_SOUND_ADD(K054539, &k054539_interface)
M1_BOARD_END

static MEMORY_READ_START( gij_sound_readmem )
	{ 0x0000, 0xebff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xfa2f, K054539_0_r },
	{ 0xfc02, 0xfc02, latch_r },
MEMORY_END

static MEMORY_WRITE_START( gij_sound_writemem )
	{ 0x0000, 0xebff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xfa2f, K054539_0_w },
	{ 0xfc00, 0xfc00, MWA_NOP },
MEMORY_END

M1_BOARD_START( gijoe )
	MDRV_NAME( "GI Joe" )
	MDRV_HWDESC( "Z80, K054539" )
	MDRV_DELAYS( 60, 10 )
	MDRV_INIT( PGX_Init )
	MDRV_SEND( XEX_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(gij_sound_readmem, gij_sound_writemem)

	MDRV_SOUND_ADD(K054539, &gij_k054539_interface)
M1_BOARD_END

static void ym_set_mixing(double left, double right)
{
	if(Machine->sample_rate) 
	{
		int l = (int)(71*left);
		int r = (int)(71*right);
		int ch;

		for(ch=0; ch<MIXER_MAX_CHANNELS; ch++) {
			const char *name = mixer_get_name(ch);
			if(name && name[0] == 'Y')
				mixer_set_stereo_volume(ch, l, r);
		}
	}
}

void pgx_timer(void)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
}

static void PGX_Init(long srate)
{
	int i;

	m1snd_addToCmdQueue(0xfb);	// stereo

	if (Machine->refcon == 2)
	{
		// Gaiapolis
		for (i=5; i<=7; i++) K054539_set_gain(0, i, 2.0);
	}
}

static void XEX_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

