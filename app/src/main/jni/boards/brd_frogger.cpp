/* Various Konami "ancestor of GX400" sound boards

   Includes Scramble, Frogger, and Time Pilot.

 */

#include "m1snd.h"

#define Z80_CLOCK (14318000/8)
#define TIMER_RATE (1.0/3500.0)

static void Frog_Init(long srate);
static void Scob_Init(long srate);
static void Frog_SendCmd(int cmda, int cmdb);

static int cmd_latch, latch_spot, amibase, amistep, amistop;

static READ_HANDLER(scob_portA_r);
static READ_HANDLER(frog_portB_r);
static READ_HANDLER(scramble_portB_r);

static int trace = 0;

struct AY8910interface frogger_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ MIXERG(80,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ scob_portA_r },
	{ frog_portB_r },
	{ 0 },
	{ 0 }
};

struct AY8910interface scobra_ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 MHz */
	/* Ant Eater clips if the volume is set higher than this */
	{ MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER), MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ 0, scob_portA_r },
	{ 0, scramble_portB_r },
	{ 0, 0 },
	{ 0, 0 }
};

struct AY8910interface timeplt_ay8910_interface =
{
	2,				/* 2 chips */
	14318180/8,		/* 1.789772727 MHz */
	{ MIXERG(30,MIXER_GAIN_2x,MIXER_PAN_CENTER), MIXERG(30,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ scob_portA_r },
	{ scramble_portB_r },
	{ 0 },
	{ 0 }
};

MEMORY_READ_START( frogger_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( frogger_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_NOP }, //frogger_filter_w },
MEMORY_END


PORT_READ_START( frogger_sound_readport )
	{ 0x40, 0x40, AY8910_read_port_0_r },
PORT_END

PORT_WRITE_START( frogger_sound_writeport )
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ 0x80, 0x80, AY8910_control_port_0_w },
PORT_END

M1_BOARD_START( frogger )
	MDRV_NAME("Frogger")
	MDRV_HWDESC("Z80, AY-3-8910")
	MDRV_DELAYS( 800, 50 )
	MDRV_INIT( Frog_Init )
	MDRV_SEND( Frog_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(frogger_sound_readmem, frogger_sound_writemem)
	MDRV_CPU_PORTS(frogger_sound_readport, frogger_sound_writeport)

	MDRV_SOUND_ADD(AY8910, &frogger_ay8910_interface)
M1_BOARD_END

static UINT8 *scobra_soundram;

static READ_HANDLER(scobra_soundram_r)
{
	return scobra_soundram[offset & 0x03ff];
}

static WRITE_HANDLER(scobra_soundram_w)
{
	scobra_soundram[offset & 0x03ff] = data;
}

MEMORY_READ_START( scobra_sound_readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x8000, 0x8fff, scobra_soundram_r },
MEMORY_END

MEMORY_WRITE_START( scobra_sound_writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x8000, 0x8fff, scobra_soundram_w, &scobra_soundram },  /* only here to initialize pointer */
	{ 0x9000, 0x9fff, MWA_NOP }, //scramble_filter_w },
MEMORY_END

PORT_READ_START( scobra_sound_readport )
	{ 0x20, 0x20, AY8910_read_port_0_r },
	{ 0x80, 0x80, AY8910_read_port_1_r },
PORT_END

PORT_WRITE_START( scobra_sound_writeport )
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x20, 0x20, AY8910_write_port_0_w },
	{ 0x40, 0x40, AY8910_control_port_1_w },
	{ 0x80, 0x80, AY8910_write_port_1_w },
PORT_END

M1_BOARD_START( scobra )
	MDRV_NAME("Scramble / Super Cobra")
	MDRV_HWDESC("Z80, AY-3-8910(x2)")
	MDRV_DELAYS( 800, 50 )
	MDRV_INIT( Scob_Init )
	MDRV_SEND( Frog_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(scobra_sound_readmem, scobra_sound_writemem)
	MDRV_CPU_PORTS(scobra_sound_readport, scobra_sound_writeport)

	MDRV_SOUND_ADD(AY8910, &scobra_ay8910_interface)
M1_BOARD_END

MEMORY_READ_START( timeplt_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
MEMORY_END

MEMORY_WRITE_START( timeplt_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
	{ 0x8000, 0x8fff, MWA_NOP }, //timeplt_filter_w },
MEMORY_END

M1_BOARD_START( timeplt )
	MDRV_NAME("Time Pilot")
	MDRV_HWDESC("Z80, AY-3-8910(x2)")
	MDRV_DELAYS( 800, 50 )
	MDRV_INIT( Scob_Init )
	MDRV_SEND( Frog_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(timeplt_sound_readmem, timeplt_sound_writemem)

	MDRV_SOUND_ADD(AY8910, &timeplt_ay8910_interface)
M1_BOARD_END

static int curtime;

static int frogger_timer[10] =
{
	0x00, 0x10, 0x08, 0x18, 0x40, 0x90, 0x88, 0x98, 0x88, 0xd0
};

static int scramble_timer[10] =
{
	0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
};

static void scob_timer(int refcon)
{
	cmd_latch++;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static READ_HANDLER(scob_portA_r)
{
//	printf("read %x from latch\n", cmd_latch);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	if (latch_spot < (amistep-1))
	{
		latch_spot++;
		timer_set(TIME_IN_MSEC(1), 0, scob_timer);
	}

	return cmd_latch;
}

static READ_HANDLER(frog_portB_r)
{
//	printf("read frog portB\n");
	return frogger_timer[curtime];
}

static READ_HANDLER(scramble_portB_r)
{
//	printf("read portB\n");
	return scramble_timer[curtime];
}

static void gx400_timer(int refcon)
{
	curtime++;
	if (curtime == 10) 
	{
		curtime = 0;
	}

	timer_set(TIMER_RATE, 0, gx400_timer);
}

static void Frog_Init(long srate)
{
	UINT8 *ROM;
	int A;

	cmd_latch = 0;

	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	ROM = memory_region(RGN_CPU1);
	for (A = 0; A < 0x0800; A++)
		ROM[A] = BITSWAP8(ROM[A],7,6,5,4,3,2,0,1);

	timer_set(TIMER_RATE, 0, gx400_timer);

	switch (Machine->refcon)
	{
		case 1:	// frogger
			amibase = 9;
			amistep = 2;
			amistop = 19;
			break;

		case 2:	// video hustler
			amibase = 9;
			amistep = 3;
			amistop = 14;
			break;
	}
}

static void Scob_Init(long srate)
{
	cmd_latch = 0;
	latch_spot = 0;

	timer_set(TIMER_RATE, 0, gx400_timer);

	switch (Machine->refcon)
	{
		case 3:	// amidar
			amibase = 19;
			amistep = 3;
			amistop = 50;
			break;

		case 4: //GAME_SCRAMBLE:
			amibase = 9;
			amistep = 2;
			amistop = 16;
			break;

		case 5: //GAME_TURTLES:
			amibase = 8;
			amistop = 38;
			amistep = 2;
			break;

		case 6: //GAME_MRKOUGAR:
			amibase = 12;
			amistop = 49;
			amistep = 3;
			break;
	}
}

static void Frog_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	latch_spot = amistep;

	if ((cmda >= amibase) && (cmda < amistop) && (((cmda-amibase)%amistep) == 0))
	{
		latch_spot = 0;
	}

	trace = 1;

 	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
