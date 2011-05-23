/* SNK "Psycho Soldier" */

#include "m1snd.h"

static int cmd_latch, snk_sound_register;

static READ_HANDLER( latch_r )
{
//	printf("Rd %x from latch\n", cmd_latch);
	snk_sound_register &= ~0xc;
//	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static void SNK_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	snk_sound_register |= 0x08 | 0x04;

//	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static WRITE_HANDLER( snk_sound_register_w ){
	snk_sound_register &= (data>>4);
}

static READ_HANDLER( snk_sound_register_r ){
	return snk_sound_register;// | 0x2; /* hack; lets chopper1 play music */
}

void snk_sound_callback0_w( int state ){ /* ? */
//	printf("IRQ0: %d\n", state);
	if( state ) snk_sound_register |= 0x01;
}

void snk_sound_callback1_w( int state ){ /* ? */
//	printf("IRQ1: %d\n", state);
	if( state ) snk_sound_register |= 0x02;
}

static MEMORY_READ_START( YM3526_Y8950_readmem_sound )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xe000, 0xe000, latch_r },
	{ 0xe800, 0xe800, YM3526_status_port_0_r },
	{ 0xf000, 0xf000, Y8950_status_port_0_r },
	{ 0xf800, 0xf800, snk_sound_register_r },
MEMORY_END

static MEMORY_WRITE_START( YM3526_Y8950_writemem_sound )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xe800, 0xe800, YM3526_control_port_0_w },
	{ 0xec00, 0xec00, YM3526_write_port_0_w },
	{ 0xf000, 0xf000, Y8950_control_port_0_w },
	{ 0xf400, 0xf400, Y8950_write_port_0_w },
	{ 0xf800, 0xf800, snk_sound_register_w },
MEMORY_END

static struct YM3526interface ym3526_interface = {
	1,			/* number of chips */
	4000000,	/* 4 MHz */
	{ 100 },		/* mixing level */
	{ snk_sound_callback0_w } /* ? */
};

static struct Y8950interface y8950_interface = {
	1,			/* number of chips */
	4000000,	/* 4 MHz */
	{ 100 },		/* mixing level */
	{ snk_sound_callback1_w }, /* ? */
	{ REGION_SOUND1 }	/* memory region */
};

static void snk_timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	
	timer_set(1.0/120.0, 0, snk_timer);
}

static void SNK_Init(long foo)
{
	timer_set(1.0/120.0, 0, snk_timer);

	snk_sound_register = 0;

	// sound driver "wake up" sequence
	m1snd_addToCmdQueue(1);
	m1snd_addToCmdQueue(13);
	m1snd_addToCmdQueue(13);
	m1snd_addToCmdQueue(103);
}

M1_BOARD_START( psychos )
	MDRV_NAME("Psycho Soldier")
	MDRV_HWDESC("Z80, YM3526, Y8950")
	MDRV_INIT( SNK_Init )
	MDRV_SEND( SNK_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(YM3526_Y8950_readmem_sound,YM3526_Y8950_writemem_sound)

	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
	MDRV_SOUND_ADD(Y8950, &y8950_interface)
M1_BOARD_END

static READ_HANDLER( snk_soundlatch_clear_r ){ /* TNK3 */
	cmd_latch = 0;
	snk_sound_register = 0;
	return 0x00;
}

static MEMORY_READ_START( YM3526_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, latch_r },
	{ 0xc000, 0xc000, snk_soundlatch_clear_r },
	{ 0xe000, 0xe000, YM3526_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( YM3526_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe000, YM3526_control_port_0_w },
	{ 0xe001, 0xe001, YM3526_write_port_0_w },
MEMORY_END

M1_BOARD_START( tnk3 )
	MDRV_NAME("T.N.K. III")
	MDRV_HWDESC("Z80, YM3526")
	MDRV_INIT( SNK_Init )
	MDRV_SEND( SNK_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(YM3526_readmem_sound,YM3526_writemem_sound)

	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
M1_BOARD_END

