/* Taito+Toaplan Slap Fight / Get Star / Tiger Heli */

#include "m1snd.h"

static unsigned char *slapfight_dpram;
static int getstar_sh_intenabled;

static void SF_SendCmd(int cmda, int cmdb)
{
	slapfight_dpram[0] = cmda;
}

WRITE_HANDLER( slapfight_dpram_w )
{
    slapfight_dpram[offset]=data;
}

READ_HANDLER( slapfight_dpram_r )
{
	return slapfight_dpram[offset];
}

WRITE_HANDLER( getstar_sh_intenable_w )
{
	getstar_sh_intenabled = 1;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0xa081, 0xa081, AY8910_read_port_0_r },
	{ 0xa091, 0xa091, AY8910_read_port_1_r },
	{ 0xc800, 0xc80f, slapfight_dpram_r },
	{ 0xc810, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0xa080, 0xa080, AY8910_control_port_0_w },
	{ 0xa082, 0xa082, AY8910_write_port_0_w },
	{ 0xa090, 0xa090, AY8910_control_port_1_w },
	{ 0xa092, 0xa092, AY8910_write_port_1_w },
	{ 0xa0e0, 0xa0e0, getstar_sh_intenable_w }, /* maybe a0f0 also -LE */
	{ 0xa0f0, 0xa0f0, MWA_NOP },
	{ 0xc800, 0xc80f, slapfight_dpram_w, &slapfight_dpram },
	{ 0xc810, 0xcfff, MWA_RAM },
MEMORY_END

static READ_HANDLER( dummy_r )
{
	return 0;
}

static struct AY8910interface ay8910_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 15, 15 },
	{ dummy_r, dummy_r },
	{ dummy_r, dummy_r },
	{ 0, 0 },
	{ 0, 0 }
};

static void sf_timer(int refcon)
{
	slapfight_dpram[1] = 0xaa;	// protocol with main CPU

	if (getstar_sh_intenabled)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	}

	timer_set(1.0/180.0, 0, sf_timer);
}

static void th_timer(int refcon)
{
	slapfight_dpram[1] = 0xaa;	// protocol with main CPU

	if (getstar_sh_intenabled)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	}

	timer_set(1.0/360.0, 0, th_timer);
}

static void SF_Init(long srate)
{
	getstar_sh_intenabled = 0;
	timer_set(1.0/180.0, 0, sf_timer);
}

static void TH_Init(long srate)
{
	getstar_sh_intenabled = 0;
	timer_set(1.0/360.0, 0, th_timer);
}

M1_BOARD_START( slapfight )
	MDRV_NAME("Slap Fight")
	MDRV_HWDESC("Z80, AY-3-8910(x2)")
	MDRV_INIT( SF_Init )
	MDRV_SEND( SF_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END

M1_BOARD_START( tigerh )
	MDRV_NAME("Tiger Heli")
	MDRV_HWDESC("Z80, AY-3-8910(x2)")
	MDRV_INIT( TH_Init )
	MDRV_SEND( SF_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END

static MEMORY_READ_START( perfrman_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8800, 0x880f, slapfight_dpram_r },
	{ 0x8810, 0x8fff, MRA_RAM },
	{ 0xa081, 0xa081, AY8910_read_port_0_r },
	{ 0xa091, 0xa091, AY8910_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( perfrman_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8800, 0x880f, slapfight_dpram_w },
	{ 0x8810, 0x8fff, MWA_RAM },	/* Shared RAM with main CPU */
	{ 0xa080, 0xa080, AY8910_control_port_0_w },
	{ 0xa082, 0xa082, AY8910_write_port_0_w },
	{ 0xa090, 0xa090, AY8910_control_port_1_w },
	{ 0xa092, 0xa092, AY8910_write_port_1_w },
	{ 0xa0e0, 0xa0e0, getstar_sh_intenable_w }, /* maybe a0f0 also -LE */
	{ 0xa0f0, 0xa0f0, MWA_NOP },
MEMORY_END

static struct AY8910interface perfrman_ay8910_interface =
{
	2,				/* 2 chips */
	16000000/8,		/* 2MHz ???, 16MHz Oscillator */
	{ 15, 15 },
	{ dummy_r, dummy_r },
	{ dummy_r, dummy_r },
	{ 0, 0 },
	{ 0, 0 }
};

M1_BOARD_START( perfman )
	MDRV_NAME("Performan")
	MDRV_HWDESC("Z80, AY-3-8910(x2)")
	MDRV_INIT( TH_Init )
	MDRV_SEND( SF_SendCmd )

	MDRV_CPU_ADD(Z80C, 2000000)
	MDRV_CPU_MEMORY(perfrman_sound_readmem,perfrman_sound_writemem)

	MDRV_SOUND_ADD(AY8910, &perfrman_ay8910_interface)
M1_BOARD_END

