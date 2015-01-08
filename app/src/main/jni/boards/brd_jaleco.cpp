/*
	Ginga NinkyouDen
	Momoko 120%
	City Connection
*/

#include "m1snd.h"

static void Ginganin_Init(long foo);
static void Ginganin_SendCmd(int cmda, int cmdb);

static void *int_timer;

static struct AY8910interface ay8910_interface =
{
	1,
	3579545 / 2,
	{ 10 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static struct Y8950interface y8950_interface = {
	1,
	3579545,
	{ 100 },
	{ NULL },
	{ REGION_SOUND1 }
};




/* Added by Takahiro Nogi. 1999/09/27 */
static UINT8 MC6840_index0;
static UINT8 MC6840_register0;
static UINT8 MC6840_index1;
static UINT8 MC6840_register1;
static int S_TEMPO = 0;
static int S_TEMPO_OLD = 0;
static int MC6809_CTR = 0;
static int MC6809_FLAG = 0;

static WRITE8_HANDLER( MC6840_control_port_0_w )
{
	/* MC6840 Emulation by Takahiro Nogi. 1999/09/27
    (This routine hasn't been completed yet.) */

	MC6840_index0 = data;

	if (MC6840_index0 & 0x80)	/* enable timer output */
	{
		if ((MC6840_register0 != S_TEMPO) && (MC6840_register0 != 0))
		{
			S_TEMPO = MC6840_register0;
		}
		MC6809_FLAG = 1;
	}
	else
	{
		MC6809_FLAG = 0;
	}
}

static WRITE8_HANDLER( MC6840_control_port_1_w )
{
	MC6840_index1 = data;
}

static WRITE8_HANDLER( MC6840_write_port_0_w )
{
	MC6840_register0 = data;
}

static WRITE8_HANDLER( MC6840_write_port_1_w )
{
	MC6840_register1 = data;
}



static int cmd_latch;

READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}


static MEMORY_READ_START( ginganin_sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1800, 0x1800, latch_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( ginganin_sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x0800, MC6840_control_port_0_w },
	{ 0x0801, 0x0801, MC6840_control_port_1_w },
	{ 0x0802, 0x0802, MC6840_write_port_0_w },
	{ 0x0803, 0x0803, MC6840_write_port_1_w },
	{ 0x2000, 0x2000, Y8950_control_port_0_w },
	{ 0x2001, 0x2001, Y8950_write_port_0_w },
	{ 0x2800, 0x2800, AY8910_control_port_0_w },
	{ 0x2801, 0x2801, AY8910_write_port_0_w },
MEMORY_END


/* Modified by Takahiro Nogi. 1999/09/27 */
static void timer_callback(int ref)
{
	if (S_TEMPO_OLD != S_TEMPO)
	{
		S_TEMPO_OLD = S_TEMPO;
		MC6809_CTR = 0;
	}

	if (MC6809_FLAG != 0)
	{
		if (MC6809_CTR > S_TEMPO)
		{
			MC6809_CTR = 0;
			cpu_set_irq_line(0, 0, HOLD_LINE);
		}
		else
		{
			MC6809_CTR++;
		}
	}
}


M1_BOARD_START( ginganin )
	MDRV_NAME("Ginga NinkyouDen")
	MDRV_HWDESC("68B09, YM2149, Y8950")
	MDRV_INIT( Ginganin_Init )
	MDRV_SEND( Ginganin_SendCmd )

	MDRV_CPU_ADD(M6809B, 1000000)
	MDRV_CPU_MEMORY(ginganin_sound_readmem, ginganin_sound_writemem)

	MDRV_SOUND_ADD(Y8950, &y8950_interface)
	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END


static void Ginganin_Init(long srate)
{
	int_timer = timer_pulse(TIME_IN_HZ(60*60), 0, timer_callback);
}

static void Ginganin_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}



/*
	Momoko 120%
*/

READ_HANDLER( momoko_latch_r )
{
	return cmd_latch;
}

static struct YM2203interface ym2203_interface =
{
	2,	/* 2 chips */
	1250000,
	{ YM2203_VOL(40,15), YM2203_VOL(40,15) },
	{ NULL , momoko_latch_r },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL }
};

//static void Momoko_Init(long foo);
static void Momoko_SendCmd(int cmda, int cmdb);

static MEMORY_READ_START( momoko_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, YM2203_status_port_0_r },
	{ 0xa001, 0xa001, YM2203_read_port_0_r },
	{ 0xc000, 0xc000, YM2203_status_port_1_r },
	{ 0xc001, 0xc001, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( momoko_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, MWA_NOP }, /* unknown */
	{ 0xa000, 0xa000, YM2203_control_port_0_w },
	{ 0xa001, 0xa001, YM2203_write_port_0_w },
	{ 0xb000, 0xb000, MWA_NOP }, /* unknown */
	{ 0xc000, 0xc000, YM2203_control_port_1_w },
	{ 0xc001, 0xc001, YM2203_write_port_1_w },
MEMORY_END



M1_BOARD_START( momoko )
	MDRV_NAME("Momoko 120%")
	MDRV_HWDESC("Z80, YM2203(x2)")
	MDRV_SEND( Momoko_SendCmd )

	MDRV_CPU_ADD(Z80C, 2500000)
	MDRV_CPU_MEMORY(momoko_sound_readmem, momoko_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END


static void Momoko_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}



/*
	City Connection
*/

READ_HANDLER( citycon_soundlatch_r )
{
	return cmd_latch;
}

READ_HANDLER( citycon_soundlatch2_r )
{
	return 0;  // TODO: sound effects
}


static struct AY8910interface citycon_ay8910_interface =
{
	1,
	1250000,
	{ 20 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static struct YM2203interface citycon_ym2203_interface =
{
	1,	/* 1 chip */
	1250000,
	{ YM2203_VOL(20,20) },
	{ citycon_soundlatch_r },
	{ citycon_soundlatch2_r },
	{ NULL },
	{ NULL },
	{ NULL }
};

static void Citycon_Init(long foo);
static void Citycon_SendCmd(int cmda, int cmdb);

static MEMORY_READ_START( citycon_sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
//  AM_RANGE(0x4002, 0x4002) AM_READ(AY8910_read_port_0_r)  /* ?? */
	{ 0x6001, 0x6001, YM2203_read_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( citycon_sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
    { 0x4000, 0x4000, AY8910_control_port_0_w},
	{ 0x4001, 0x4001, AY8910_write_port_0_w},
	{ 0x6000, 0x6000, YM2203_control_port_0_w},
	{ 0x6001, 0x6001, YM2203_write_port_0_w},
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


M1_BOARD_START( citycon )
	MDRV_NAME("City Connection")
	MDRV_HWDESC("6809, AY8910, YM2203")
	MDRV_INIT( Citycon_Init )
	MDRV_SEND( Citycon_SendCmd )

	MDRV_CPU_ADD(M6809B, 640000)
	MDRV_CPU_MEMORY(citycon_sound_readmem, citycon_sound_writemem)

	MDRV_SOUND_ADD(AY8910, &citycon_ay8910_interface)
	MDRV_SOUND_ADD(YM2203, &citycon_ym2203_interface)
M1_BOARD_END


static void citycon_timer_callback(int ref)
{
	cpu_set_irq_line(0, 0, HOLD_LINE);
}

static void Citycon_Init(long srate)
{
	int_timer = timer_pulse(TIME_IN_HZ(60), 0, citycon_timer_callback);
}

static void Citycon_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
