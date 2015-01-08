#include "m1snd.h"

#define OSC_A	(32215900)	// System 32 clock A is 32215900 Hz
#define Z80_CLOCK (OSC_A/4)
#define MPCM_CLOCK (OSC_A/4)
#define YM_M32_CLOCK (OSC_A/4)

static void M32_SendCmd(int cmda, int cmdb);

static void irq_handler(int irq)
{
	cpu_set_irq_line( 0, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}
						  
static struct YM2612interface sys32_ym3438_interface =
{
	1,
	Z80_CLOCK,
	{ MIXER(60,MIXER_PAN_LEFT), MIXER(60,MIXER_PAN_RIGHT) },
	{ 0 },	{ 0 },	{ 0 },	{ 0 },
	{ irq_handler },
};

static struct MultiPCM_interface sys32_multipcm_interface =
{
	1,
	{ MPCM_CLOCK },
	{ MULTIPCM_MODE_MULTI32 },
	{ (512*1024) },
	{ RGN_SAMP1 },
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) }
};

static struct MultiPCM_interface sys32_stereo_multipcm_interface =
{
	1,
	{ MPCM_CLOCK },
	{ MULTIPCM_MODE_MULTI32 },
	{ (512*1024) },
	{ RGN_SAMP1 },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) }
};

static struct MultiPCM_interface sys32_scross_multipcm_interface =
{
	1,
	{ MPCM_CLOCK },
	{ MULTIPCM_MODE_STADCROSS },
	{ (512*1024) },
	{ RGN_SAMP1 },
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) }
};

static UINT8 *sys32_SoundMemBank;
static data8_t *system32_shared_ram;

static READ_HANDLER( system32_bank_r )
{
	return sys32_SoundMemBank[offset];
}

// the Z80's work RAM is fully shared with the V60 or V70 and battery backed up.
static READ_HANDLER( sys32_shared_snd_r )
{
	data8_t *RAM = system32_shared_ram;

	return RAM[offset];
}

static WRITE_HANDLER( sys32_shared_snd_w )
{
	data8_t *RAM = system32_shared_ram;

	RAM[offset] = data;
}

static MEMORY_READ_START( sound_readmem_32 )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xbfff, system32_bank_r },
	{ 0xc000, 0xdfff, MultiPCM_reg_0_r },
	{ 0xe000, 0xffff, sys32_shared_snd_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_32 )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xdfff, MultiPCM_reg_0_w },
	{ 0xe000, 0xffff, sys32_shared_snd_w, &system32_shared_ram },
MEMORY_END

static WRITE_HANDLER( sys32_soundbank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int Bank;

	Bank = data * 0x2000;

	sys32_SoundMemBank = &RAM[Bank];
}

static int s32_f1_prot;

static READ_HANDLER( sys32_sound_prot_r )
{
	return s32_f1_prot;
}

static WRITE_HANDLER( sys32_sound_prot_w )
{
	s32_f1_prot = data;
}

static PORT_READ_START( sound_readport_32 )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
	{ 0xf1, 0xf1, sys32_sound_prot_r },
PORT_END

static PORT_WRITE_START( sound_writeport_32 )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0xa0, 0xa0, sys32_soundbank_w },
	{ 0xb0, 0xb0, MultiPCM_bank_0_w },
	{ 0xc1, 0xc1, IOWP_NOP },
	{ 0xf1, 0xf1, sys32_sound_prot_w },
PORT_END

static void M32_Init(long foo)
{
	if (Machine->refcon == 1)
	{
		m1snd_addToCmdQueue(0);
		m1snd_addToCmdQueue(0);
		m1snd_addToCmdQueue(0x1f);
	}
}

M1_BOARD_START( segamulti32 )
	MDRV_NAME( "System 32 Multi" )
	MDRV_HWDESC( "Z80, YM3438, Sega MultiPCM" )
	MDRV_DELAYS( 30, 15 )
	MDRV_INIT( M32_Init )
	MDRV_SEND( M32_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem_32, sound_writemem_32)
	MDRV_CPU_PORTS(sound_readport_32, sound_writeport_32)

	MDRV_SOUND_ADD(YM3438, &sys32_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, &sys32_multipcm_interface)
M1_BOARD_END

M1_BOARD_START( segamulti32stereo )
	MDRV_NAME( "System 32 Multi (stereo)" )
	MDRV_HWDESC( "Z80, YM3438, Sega MultiPCM" )
	MDRV_DELAYS( 30, 15 )
	MDRV_SEND( M32_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem_32, sound_writemem_32)
	MDRV_CPU_PORTS(sound_readport_32, sound_writeport_32)

	MDRV_SOUND_ADD(YM3438, &sys32_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, &sys32_stereo_multipcm_interface)
M1_BOARD_END

M1_BOARD_START( segamulti32scross )
	MDRV_NAME( "Stadium Cross" )
	MDRV_HWDESC( "Z80, YM3438, Sega MultiPCM" )
	MDRV_DELAYS( 30, 15 )
	MDRV_SEND( M32_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem_32, sound_writemem_32)
	MDRV_CPU_PORTS(sound_readport_32, sound_writeport_32)

	MDRV_SOUND_ADD(YM3438, &sys32_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, &sys32_scross_multipcm_interface)
M1_BOARD_END

static void M32_SendCmd(int cmda, int cmdb)
{
	system32_shared_ram[0x800] = cmda&0xff;
}

