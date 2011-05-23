/* Bally/Midway Super Sound I/O */

#include "m1snd.h"

static void SSIO_Init(long srate);
static void SSIO_SendCmd(int cmda, int cmdb);

#define TIMER_RATE (1.0/780.0)	// 780 times per second

static UINT8 ssio_data[4];
static UINT8 ssio_status;
static UINT8 ssio_duty_cycle[2][3];

static int toggle = 0x80;

static READ_HANDLER( input_port_5_r )
{
	return 0xff;
}

static WRITE_HANDLER( ssio_status_w )
{
	ssio_status = data;
}

static READ_HANDLER( ssio_data_r )
{
	return ssio_data[offset];
}

static void ssio_update_volumes(void)
{
	int chip, chan;
	for (chip = 0; chip < 2; chip++)
		for (chan = 0; chan < 3; chan++)
			AY8910_set_volume(chip, chan, (ssio_duty_cycle[chip][chan] ^ 15) * 100 / 15);
}

static WRITE_HANDLER( ssio_porta0_w )
{
	ssio_duty_cycle[0][0] = data & 15;
	ssio_duty_cycle[0][1] = data >> 4;
	ssio_update_volumes();
}

static WRITE_HANDLER( ssio_portb0_w )
{
	ssio_duty_cycle[0][2] = data & 15;
	ssio_update_volumes();
}

static WRITE_HANDLER( ssio_porta1_w )
{
	ssio_duty_cycle[1][0] = data & 15;
	ssio_duty_cycle[1][1] = data >> 4;
	ssio_update_volumes();
}

static WRITE_HANDLER( ssio_portb1_w )
{
	ssio_duty_cycle[1][2] = data & 15;
	mixer_sound_enable_global_w(!(data & 0x80));
	ssio_update_volumes();
}

/********* external interfaces ***********/

READ_HANDLER( ssio_status_r )
{
	return ssio_status;
}

static struct AY8910interface ssio_ay8910_interface =
{
	2,			/* 2 chips */
	2000000,	/* 2 MHz ?? */
	{ MIXER(33,MIXER_PAN_LEFT), MIXER(33,MIXER_PAN_RIGHT) },	/* dotron clips with anything higher */
	{ 0 },
	{ 0 },
	{ ssio_porta0_w, ssio_porta1_w },
	{ ssio_portb0_w, ssio_portb1_w }
};

static MEMORY_READ_START( ssio_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, ssio_data_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0xf000, 0xf000, input_port_5_r },
MEMORY_END

static MEMORY_WRITE_START( ssio_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, ssio_status_w },
	{ 0xe000, 0xe000, MWA_NOP },
MEMORY_END

M1_BOARD_START( ssio )
	MDRV_NAME("Bally/Midway Super Sound I/O")
	MDRV_HWDESC("Z80, AY-3-8910(x2)")
	MDRV_DELAYS( 400, 200 )
	MDRV_INIT( SSIO_Init )
	MDRV_SEND( SSIO_SendCmd )

	MDRV_CPU_ADD(Z80C, 2000000)
	MDRV_CPU_MEMORY(ssio_readmem,ssio_writemem)

	MDRV_SOUND_ADD(AY8910, &ssio_ay8910_interface)
M1_BOARD_END

static void timer(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void SSIO_Init(long srate)
{
	int i;

	timer_set(TIMER_RATE, 0, timer);

	// init the latches
	for (i = 0; i < 4; i++)
	{
		ssio_data[i] = 0;
	}
	ssio_status = 0;

	// init sequence logged from MAME
	m1snd_addToCmdQueue(0xff);
	m1snd_addToCmdQueue(0x55);
	m1snd_addToCmdQueue(0xaa);
	m1snd_addToCmdQueue(0x02);
	m1snd_addToCmdQueue(0xee);
}

static void SSIO_SendCmd(int cmda, int cmdb)
{
	if ((cmda == 0xff) || (cmda == 0x55) || (cmda == 0xaa))
	{
		ssio_data[0] = ssio_data[1] = ssio_data[2] = ssio_data[3] = cmda;
		return;
	}

	if (cmda == 0xee)
	{
		ssio_data[1] = 2;
		ssio_data[2] = 1;
		ssio_data[3] = 0;
	}
	else
	{
		ssio_data[1] = cmda;
		ssio_data[2] = ssio_data[3] = 0;
	}

	ssio_data[0] = toggle;
	toggle ^= 0x80;
}
