#include "m1snd.h"

#define OSC_A	(32215900)	// System 32 clock A is 32215900 Hz
#define OSC_B   (50000000)	// clock B is 50 MHz
#define Z80_CLOCK (OSC_A/4)
#define S32_YM_CLOCK (OSC_A/4)
#define S32_RF_CLOCK (OSC_B/4)

#define S18_YM_CLOCK (8000000)
#define S18_RF_CLOCK (10000000)
#define S18_Z80_CLOCK (4000000)

static void S18_SendCmd(int cmda, int cmdb);
static void S32_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);

static int cmd_latch;
static void YM_IRQHandler(int irq);

struct RF5C68interface sys18_rf5c68_interface = {
  S18_RF_CLOCK,
  100
};

struct RF5C68interface sys32_rf5c68_interface = 
{
  S32_RF_CLOCK,
  40
};

struct YM2612interface sys18_ym3438_interface =
{
	2,	/* 2 chips */
	S18_YM_CLOCK,
	{ MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT) },
	{ 0 },	{ 0 },	{ 0 },	{ 0 }
};

struct YM2612interface sys32_ym3438_interface =
{
	2,	/* 2 chips */
	S32_YM_CLOCK,
	{ MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) },
	{ 0 },	{ 0 },	{ 0 },	{ 0 },
	{ YM_IRQHandler }
};

static UINT8 *sys32_SoundMemBank, *sys32_shared_ram, s32_f1_prot;

static READ_HANDLER( system32_bank_r )
{
	return sys32_SoundMemBank[offset];
}

// some games require that port f1 be a magic echo-back latch.
// thankfully, it's not required to do any math or anything on the values.
static READ_HANDLER( sys32_sound_prot_r )
{
	return s32_f1_prot;
}

static WRITE_HANDLER( sys32_sound_prot_w )
{
	s32_f1_prot = data;
}

static MEMORY_READ_START( sound_readmem_32 )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xbfff, system32_bank_r },
	{ 0xd000, 0xdfff, RF5C68_r },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_32 )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc008, RF5C68_reg_w },
	{ 0xd000, 0xdfff, RF5C68_w },
	{ 0xe000, 0xffff, MWA_RAM, &sys32_shared_ram },
MEMORY_END

static int s32_blo, s32_bhi;

static void s32_recomp_bank(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int Bank=0;

	Bank = (s32_blo & 0x3f) | ((s32_bhi & 0x04) << 4) | ((s32_bhi & 0x03)<<7);

	sys32_SoundMemBank = &RAM[Bank*0x2000];
}

static WRITE_HANDLER( sys18_soundbank_w )
{
	/* select access bank for a000~bfff */
	unsigned char *RAM = memory_region(REGION_CPU1);

	int Bank = (data * 0x2000);
	sys32_SoundMemBank = &RAM[Bank];
}

static WRITE_HANDLER( sys32_soundbank_lo_w )
{
	s32_blo = data;
	s32_recomp_bank();
}

static WRITE_HANDLER( sys32_soundbank_hi_w )
{
	s32_bhi = data;
	s32_recomp_bank();
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static PORT_READ_START( sound_readport_32 )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
	{ 0x90, 0x90, YM2612_status_port_1_A_r },
	{ 0xf1, 0xf1, sys32_sound_prot_r },
PORT_END

static PORT_WRITE_START( sound_writeport_32 )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0x90, 0x90, YM2612_control_port_1_A_w },
	{ 0x91, 0x91, YM2612_data_port_1_A_w },
	{ 0x92, 0x92, YM2612_control_port_1_B_w },
	{ 0x93, 0x93, YM2612_data_port_1_B_w },
	{ 0xa0, 0xa0, sys32_soundbank_lo_w },
	{ 0xb0, 0xb0, sys32_soundbank_hi_w },
	{ 0xc1, 0xc1, IOWP_NOP },
	{ 0xf1, 0xf1, sys32_sound_prot_w },
PORT_END

static PORT_READ_START( sound_readport_18 )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
	{ 0xc0, 0xc0, latch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_18 )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0x90, 0x90, YM2612_control_port_1_A_w },
	{ 0x91, 0x91, YM2612_data_port_1_A_w },
	{ 0x92, 0x92, YM2612_control_port_1_B_w },
	{ 0x93, 0x93, YM2612_data_port_1_B_w },
	{ 0xa0, 0xa0, sys18_soundbank_w },
	{ 0xc0, 0xc0, IOWP_NOP },
PORT_END

M1_BOARD_START( segasys18 )
	MDRV_NAME("System 18")
	MDRV_HWDESC("Z80, YM3438(x2), RF5c68")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( S18_SendCmd )

	MDRV_CPU_ADD(Z80C, S18_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem_32,sound_writemem_32)
	MDRV_CPU_PORTS(sound_readport_18,sound_writeport_18)

	MDRV_SOUND_ADD(YM3438, &sys18_ym3438_interface)
	MDRV_SOUND_ADD(RF5C68, &sys18_rf5c68_interface)
M1_BOARD_END

M1_BOARD_START( segasys32 )
	MDRV_NAME("System 32")
	MDRV_HWDESC("Z80, YM3438(x2), RF5c68")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( S32_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem_32,sound_writemem_32)
	MDRV_CPU_PORTS(sound_readport_32,sound_writeport_32)

	MDRV_SOUND_ADD(YM3438, &sys32_ym3438_interface)
	MDRV_SOUND_ADD(RF5C68, &sys32_rf5c68_interface)
M1_BOARD_END

static void YM_IRQHandler(int irq)
{
	cpu_set_irq_line( 0, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static void S18_SendCmd(int cmda, int cmdb)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	cmd_latch = cmda;
}

static void S32_SendCmd(int cmda, int cmdb)
{
	switch (Machine->refcon)
	{
		case 1: //GAME_RADMOBILE:
			sys32_shared_ram[0] = cmda;
			sys32_shared_ram[0x10] = 0;
			break;

		case 2: //GAME_JURASSICPARK:
			if (cmda == 0xff)	// stop?
			{
				sys32_shared_ram[0x700] = 0;
				sys32_shared_ram[0x701] = 0x80;
			}
			else
			{
				sys32_shared_ram[0x700] = cmda%80;
				sys32_shared_ram[0x701] = 0x81 + (cmda/80);
			}
			break;

		case 3: //GAME_ALIEN3:
//		case GAME_DBZVRVS:
			if (cmda == 0xff)	// stop?
			{
				sys32_shared_ram[0x700] = 3;
				sys32_shared_ram[0x701] = 0x80;
			}
			else
			{
				sys32_shared_ram[0x700] = cmda;
				sys32_shared_ram[0x701] = 0x81;
			}
			break;

		default:
			sys32_shared_ram[0] = cmda;
			break;
	}

	cmd_latch = cmda;
}
