#include "m1snd.h"

#define HC_Z80_CLOCK (3579545) 
#define HC_YM_CLOCK (3579545) 

static void HC_SendCmd(int cmda, int cmdb);
static void HCVolume_callback(int v);

static int cmd_latch;

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* memory regions */
	{ K007232_VOL(44,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },	/* volume */
	{ HCVolume_callback }	/* external port callback */
};

static struct K007232_interface kitten_k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER) },	/* volume */
	{ HCVolume_callback }	/* external port callback */
};

static struct YM3812interface ym3812_interface =
{
	1,
	3579545,
	{ 70 },
};

static struct YM3812interface kitten_ym3812_interface =
{
	1,
	3579545,
	{ 90 },
};

static struct k051649_interface k051649_interface =
{
	3579545/2,	/* Clock */
	30,			/* Volume */
};

static void HCVolume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static READ_HANDLER( latch_r )
{
//	printf("read from command (%d) at %x\n", cmd_latch, addr);
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( sound_bank_w )
{
	int bank_A=(data&0x3);
	int bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );
}

static MEMORY_READ_START( hc_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, YM3812_status_port_0_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xd000, 0xd000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( hc_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x987f, K051649_waveform_w },
	{ 0x9880, 0x9889, K051649_frequency_w },
	{ 0x988a, 0x988e, K051649_volume_w },
	{ 0x988f, 0x988f, K051649_keyonoff_w },
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, sound_bank_w }, /* 7232 bankswitch */
MEMORY_END

M1_BOARD_START( konamihcastle )
	MDRV_NAME( "Haunted Castle" )
	MDRV_HWDESC( "Z80, YM3812, K007232, K051649 (SCC1)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( HC_SendCmd )

	MDRV_CPU_ADD(Z80C, HC_Z80_CLOCK)
	MDRV_CPU_MEMORY(hc_readmem, hc_writemem)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
	MDRV_SOUND_ADD(K051649, &k051649_interface)
M1_BOARD_END

M1_BOARD_START( kittenk )
	MDRV_NAME( "Kitten Kaboodle" )
	MDRV_HWDESC( "Z80, YM3812, K007232, K051649 (SCC1)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( HC_SendCmd )

	MDRV_CPU_ADD(Z80C, HC_Z80_CLOCK)
	MDRV_CPU_MEMORY(hc_readmem, hc_writemem)

	MDRV_SOUND_ADD(YM3812, &kitten_ym3812_interface)
	MDRV_SOUND_ADD(K007232, &kitten_k007232_interface)
	MDRV_SOUND_ADD(K051649, &k051649_interface)
M1_BOARD_END

static struct YM3812interface blym3812_interface =
{
	2,				/* 2 chips */
	3000000,		/* ? */
	{ 100, 100 },
	{ 0, 0 },
};

static MEMORY_READ_START( battlnts_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },		    /* ROM 777c01.rom */
	{ 0x8000, 0x87ff, MRA_RAM },		    /* RAM */
	{ 0xa000, 0xa000, YM3812_status_port_0_r }, /* YM3812 (chip 1) */
	{ 0xc000, 0xc000, YM3812_status_port_1_r }, /* YM3812 (chip 2) */
	{ 0xe000, 0xe000, latch_r },	    		/* soundlatch_r */
MEMORY_END

static MEMORY_WRITE_START( battlnts_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },			/* ROM 777c01.rom */
	{ 0x8000, 0x87ff, MWA_RAM },			/* RAM */
	{ 0xa000, 0xa000, YM3812_control_port_0_w },	/* YM3812 (chip 1) */
	{ 0xa001, 0xa001, YM3812_write_port_0_w },	/* YM3812 (chip 1) */
	{ 0xc000, 0xc000, YM3812_control_port_1_w },	/* YM3812 (chip 2) */
	{ 0xc001, 0xc001, YM3812_write_port_1_w },	/* YM3812 (chip 2) */
MEMORY_END

M1_BOARD_START( blantis )
	MDRV_NAME( "Battlantis" )
	MDRV_HWDESC( "Z80, YM3812(x2)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( HC_SendCmd )

	MDRV_CPU_ADD(Z80C, HC_Z80_CLOCK)
	MDRV_CPU_MEMORY(battlnts_readmem_sound, battlnts_writemem_sound)

	MDRV_SOUND_ADD(YM3812, &blym3812_interface)
M1_BOARD_END
static void HC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
