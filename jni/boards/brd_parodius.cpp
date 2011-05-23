/* Konami Parodius / Asterix / Lightning Fighters / Simpsons / Rollergames sound */

#include "m1snd.h"

#define Z80_CLOCK (3579545)
#define YM_CLOCK (3579545)

static struct YM3812interface ym3812_interface =
{
	1,
	3579545,
	{ 100 },
	{ 0 },
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(70,MIXER_PAN_LEFT,70,MIXER_PAN_RIGHT) },
	{ 0 },
};

static struct K053260_interface k053260_interface =
{
	1,
	{ 3579545 },
	{ RGN_SAMP1 }, /* memory region */
	{ { MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT) } },
};

static struct YM2151interface astym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
};

static struct K053260_interface astk053260_interface =
{
	1,
	{ 4000000 },
	{ RGN_SAMP1 }, /* memory region */
	{ { MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) } },
};

static struct YM2151interface ssrym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(90,MIXER_PAN_LEFT,90,MIXER_PAN_RIGHT) },
	{ 0 },
};

static struct K053260_interface ssrk053260_interface =
{
	1,
	{ 3579545 },
	{ RGN_SAMP1 }, /* memory region */
	{ { MIXER(65,MIXER_PAN_LEFT), MIXER(65,MIXER_PAN_RIGHT) } },
};

void PFriendsInit(long foo)
{
	m1snd_addToCmdQueue(0xfb);	// set stereo mode
}

static READ_HANDLER( K053260_par_0_r )
{
	if (offset == 0)
	{
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}

	return K053260_0_r(offset);
}

static void Par_SendCmd(int cmda, int cmdb)
{
	K053260_0_w(0, cmda);	// comms go through the '260
	K053260_0_w(1, 0);
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static void nmi_callback(int param)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static WRITE_HANDLER( sound_arm_nmi_w )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	timer_set(TIME_IN_USEC(50),0,nmi_callback);	/* kludge until the K053260 is emulated correctly */
}

static MEMORY_READ_START( parodius_readmem_sound )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_par_0_r },
MEMORY_END

static MEMORY_WRITE_START( parodius_writemem_sound )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xf811, 0xf811, YM2151_data_port_0_w },	/* mirror for thndrx2 */
	{ 0xfa00, 0xfa00, sound_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
MEMORY_END

M1_BOARD_START( parodius )
	MDRV_NAME( "Parodius" )
	MDRV_HWDESC( "Z80, YM2151, K053260")
	MDRV_DELAYS( 2800, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(parodius_readmem_sound,parodius_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K053260, &k053260_interface)
M1_BOARD_END

static WRITE_HANDLER( z80_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	offset = (data & 7) * 0x4000;

	cpu_setbank( 2, &RAM[ offset ] );
}

static MEMORY_READ_START( simp_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_par_0_r },
MEMORY_END

static MEMORY_WRITE_START( simp_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, sound_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
	{ 0xfe00, 0xfe00, z80_bankswitch_w },
MEMORY_END

M1_BOARD_START( simpsons )
	MDRV_NAME( "The Simpsons" )
	MDRV_HWDESC( "Z80, YM2151, K053260")
	MDRV_DELAYS( 300, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(simp_readmem,simp_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K053260, &k053260_interface)
M1_BOARD_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfa00, 0xfa2f, K053260_par_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },	// mirror for ssriders
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa2f, K053260_0_w },
	{ 0xfc00, 0xfc00, sound_arm_nmi_w },
	{ 0xfe00, 0xfe00, YM2151_register_port_0_w },
MEMORY_END

M1_BOARD_START( asterix )
	MDRV_NAME( "Asterix" )
	MDRV_HWDESC( "Z80, YM2151, K053260")
	MDRV_DELAYS( 1200, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2151, &astym2151_interface)
	MDRV_SOUND_ADD(K053260, &astk053260_interface)
M1_BOARD_END

M1_BOARD_START( ssriders )
	MDRV_NAME( "Sunset Riders" )
	MDRV_HWDESC( "Z80, YM2151, K053260")
	MDRV_DELAYS( 1200, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ssrym2151_interface)
	MDRV_SOUND_ADD(K053260, &ssrk053260_interface)
M1_BOARD_END

static MEMORY_READ_START( punkshot_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_par_0_r },
MEMORY_END

static MEMORY_WRITE_START( punkshot_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, sound_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
MEMORY_END

M1_BOARD_START( punkshot )
	MDRV_NAME( "Punk Shot" )
	MDRV_HWDESC( "Z80, YM2151, K053260")
	MDRV_DELAYS( 1200, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(punkshot_s_readmem,punkshot_s_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K053260, &k053260_interface)
M1_BOARD_END

static MEMORY_READ_START( lgtnfght_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa001, 0xa001, YM2151_status_port_0_r },
	{ 0xc000, 0xc02f, K053260_par_0_r },
MEMORY_END

static MEMORY_WRITE_START( lgtnfght_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM2151_register_port_0_w },
	{ 0xa001, 0xa001, YM2151_data_port_0_w },
	{ 0xc000, 0xc02f, K053260_0_w },
MEMORY_END

M1_BOARD_START( lfighters )
	MDRV_NAME( "Lightning Fighters" )
	MDRV_HWDESC( "Z80, YM2151, K053260")
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(lgtnfght_s_readmem,lgtnfght_s_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K053260, &k053260_interface)
M1_BOARD_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa02f, K053260_par_0_r },
	{ 0xc000, 0xc000, YM3812_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa02f, K053260_0_w },
	{ 0xc000, 0xc000, YM3812_control_port_0_w },
	{ 0xc001, 0xc001, YM3812_write_port_0_w },
	{ 0xfc00, 0xfc00, sound_arm_nmi_w },
MEMORY_END

M1_BOARD_START( rollergames )
	MDRV_NAME( "Rollergames" )
	MDRV_HWDESC( "Z80, YM3812, K053260")
	MDRV_DELAYS( 1200, 15 )
	MDRV_INIT( PFriendsInit )
	MDRV_SEND( Par_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(K053260, &k053260_interface)
M1_BOARD_END

