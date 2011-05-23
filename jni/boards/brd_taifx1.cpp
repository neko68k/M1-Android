// Taito FX-1 and similar Taito boards (F2 system, many others)

#include "m1snd.h"
#include "taitosnd.h"

#define FX1_Z80_CLOCK (4000000)
#define FX1_YM_CLOCK (8000000)

static void ET_Init(long srate);
static void FX_Init(long srate);
static void FX_SendCmd2(int cmda, int cmdb);
static void F2_SendCmd(int cmda, int cmdb);
static void ET_SendCmd(int cmda, int cmdb);
static void irqhandler(int irq);

static WRITE_HANDLER( portA_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankofs = (data & 0x03) * 0x4000;
	cpu_setbank(7, &RAM[bankofs]);
}

static struct YM2203interface ym2203_interface_triple =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(80,20) },
	{ 0 },
	{ 0 },
	{ portA_w },
	{ 0 },
	{ irqhandler }
};

static struct YM2203interface ym2203_interface =
{
	1,
	3000000,				/* ?? */
	{ YM2203_VOL(80,25) },	/* ?? */
	{ 0 },
	{ 0 },
	{ portA_w },
	{ 0 },
	{ irqhandler }
};

static struct YM2203interface vfym2203_interface =
{
	1,
	4000000,				/* ?? */
	{ YM2203_VOL(80,25) },	/* ?? */
	{ 0 },
	{ 0 },
	{ portA_w },
	{ 0 },
	{ irqhandler }
};

static struct YM2610interface ym2610_interface =
{
	1,
	FX1_YM_CLOCK,	/* 8 MHz??? */
	{ MIXERG(6,MIXER_GAIN_1x,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ym2610_mono_interface =
{
	1,
	FX1_YM_CLOCK,	/* 8 MHz??? */
	{ MIXERG(50,MIXER_GAIN_4x,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ YM3012_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static struct YM2610interface Zsys_ym2610_interface =
{
	1,
	FX1_YM_CLOCK,	/* 8 MHz??? */
	{ MIXERG(50,MIXER_GAIN_4x,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ YM3012_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static WRITE_HANDLER( sound_bankswitch_w )
{
	int banknum = data & 7;
	cpu_setbank( 2, memory_region(REGION_CPU1) + (banknum * 0x4000));
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xe600, 0xe600, MWA_NOP }, 
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },	/* ?? */
MEMORY_END

static MEMORY_READ_START( fhawk_3_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK7 },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0xe001, 0xe001, taitosound_slave_comm_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( fhawk_3_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xe000, 0xe000, taitosound_slave_port_w },
	{ 0xe001, 0xe001, taitosound_slave_comm_w },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
MEMORY_END

static MEMORY_READ_START( masterw_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK7 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( masterw_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END

static MEMORY_READ_START( vf_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, taitosound_slave_comm_r },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( vf_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taitosound_slave_port_w },
	{ 0x8801, 0x8801, taitosound_slave_comm_w },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0x9800, 0x9800, MWA_NOP },    /* ? */
MEMORY_END

static void irqhandler(int irq)
{
	cpu_set_irq_line( 0, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static void FX_Init(long srate)
{
	m1snd_addToCmdQueue(0xffff);
	m1snd_addToCmdQueue(0xeef);
}

static void ET_Init(long srate)
{
	taitosound_port_w(0, 4);
	taitosound_comm_w(0, 0xf);
	taitosound_port_w(0, 4);
	taitosound_comm_w(0, 0);
}

static void FX_SendCmd2(int cmda, int cmdb)
{
	// logged outta ZiNc...
	taitosound_port_w(0, 4);
	taitosound_port_w(0, 0);
	taitosound_comm_w(0, 0);
	taitosound_port_w(0, 1);
	taitosound_comm_w(0, 0);
	taitosound_port_w(0, 2);
	taitosound_comm_w(0, 0);
	taitosound_port_w(0, 3);
	taitosound_comm_w(0, 0);
	taitosound_port_w(0, 4);
	taitosound_port_w(0, 0);
	taitosound_comm_w(0, cmda&0xf);
	taitosound_port_w(0, 1);
	taitosound_comm_w(0, (cmda>>4)&0xf);
	taitosound_port_w(0, 2);
	taitosound_comm_w(0, (cmda>>8)&0xf);
	taitosound_port_w(0, 3);
	taitosound_comm_w(0, (cmda>>12)&0xf);
}

static void F2_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xffff)
	{
		taitosound_port_w(0, 4);
		taitosound_comm_w(0, 0);
		taitosound_port_w(0, 0);
		taitosound_comm_w(0, 0);
	}
	else
	{
		taitosound_port_w(0, 4);
		taitosound_port_w(0, 0);
		taitosound_comm_w(0, cmda&0xf);
		taitosound_comm_w(0, (cmda&0xf0)>>4);
	}
}

static void ET_SendCmd(int cmda, int cmdb)
{
	int i;

	if (cmda == 0xffff)
	{
		taitosound_port_w(0, 4);
		taitosound_comm_w(0, 0);
		taitosound_port_w(0, 0);
		taitosound_comm_w(0, 0);
	}
	else
	{
		// I am not making this up.  The game actually does this.
		for (i = 0; i < 374; i++)
		{
			taitosound_port_w(0, 4);
		}
		taitosound_port_w(0, 0);
		taitosound_port_w(0, 2);
		taitosound_comm_w(0, (cmda&0xf0)>>4);
		taitosound_comm_w(0, cmda&0xf);
	}
}

M1_BOARD_START( taitofx1 )
	MDRV_NAME("Taito FX-1 System")
	MDRV_HWDESC("Z80, YM2610B")
	MDRV_INIT( FX_Init )
	MDRV_SEND( FX_SendCmd2 )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2610B, &ym2610_interface)
M1_BOARD_END

M1_BOARD_START( taitof2 )
	MDRV_NAME("Taito AIR/B/Dual68k/H/F2 Systems")
	MDRV_HWDESC("Z80, YM2610 or YM2610B")
	MDRV_INIT( FX_Init )
	MDRV_SEND( F2_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2610B, &ym2610_interface)
M1_BOARD_END

M1_BOARD_START( taitof2mono )
	MDRV_NAME("Taito AIR/B/Dual68k/H/F2 Systems (mono)")
	MDRV_HWDESC("Z80, YM2610 or YM2610B")
	MDRV_INIT( FX_Init )
	MDRV_SEND( F2_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2610B, &ym2610_mono_interface)
M1_BOARD_END

M1_BOARD_START( easttech )
	MDRV_NAME("Balloon Bros")
	MDRV_HWDESC("Z80, YM2610")
	MDRV_INIT( ET_Init )
	MDRV_SEND( ET_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2610B, &ym2610_interface)
M1_BOARD_END

M1_BOARD_START( fhawk )
	MDRV_NAME("Fighting Hawk")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( FX_Init )
	MDRV_SEND( F2_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(fhawk_3_readmem,fhawk_3_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface_triple)
M1_BOARD_END

M1_BOARD_START( masterw )
	MDRV_NAME("Master of Weapons")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( FX_Init )
	MDRV_SEND( F2_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(masterw_sound_readmem,masterw_sound_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

M1_BOARD_START( volfied )
	MDRV_NAME("Volfied")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( FX_Init )
	MDRV_SEND( F2_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(vf_readmem,vf_writemem)

	MDRV_SOUND_ADD(YM2203, &vfym2203_interface)
M1_BOARD_END

M1_BOARD_START( taitoz )
	MDRV_NAME("Taito Z System")
	MDRV_HWDESC("Z80, YM2610 or YM2610B")
	MDRV_INIT( FX_Init )
	MDRV_SEND( F2_SendCmd )
	MDRV_DELAYS( 120, 120 )

	MDRV_CPU_ADD(Z80C, FX1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2610B, &Zsys_ym2610_interface)
M1_BOARD_END

