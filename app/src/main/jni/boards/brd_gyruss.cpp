/* Gyruss board */

#include "m1snd.h"

#define Z80_CLOCK (14318180/4)
#define AY_CLOCK (14318180/8)
#define I8039_CLOCK (8000000/15)	// 8 mhz crystal

#define TIMER_RATE TIME_IN_HZ(Z80_CLOCK/1024.0) // around 3500 Hz...

static void GY_Init(long srate);
static void GY_SendCmd(int cmda, int cmdb);

static READ_HANDLER(gy_portA_r);

static int cmd_latch, i8039_latch, cpucycle = 0;

static struct AY8910interface ay8910_interface =
{
	5,	/* 5 chips */
	14318180/8,	/* 1.789772727 MHz */
	{ MIXERG(10,MIXER_GAIN_4x,MIXER_PAN_RIGHT), MIXERG(10,MIXER_GAIN_4x,MIXER_PAN_LEFT),
 	  MIXERG(40,MIXER_GAIN_4x,MIXER_PAN_RIGHT), MIXERG(40,MIXER_GAIN_4x,MIXER_PAN_RIGHT), MIXERG(40,MIXER_GAIN_4x,MIXER_PAN_LEFT) },
	/*  R       L   |   R       R       L */
	/*   effects    |         music       */
	{ 0, 0, gy_portA_r },
	{ 0 },
	{ 0 },
//	{ gyruss_filter0_w, gyruss_filter1_w }
};

static struct DACinterface dac_interface =
{
	1,
	{ MIXER(50,MIXER_PAN_LEFT) }
};

static WRITE_HANDLER( gyruss_i8039_irq_w )
{
	i8039_set_irq_line(0, HOLD_LINE);
}

static WRITE_HANDLER( latch2_w )
{
	i8039_latch = data;
}

static READ_HANDLER( latch_r )
{
//	printf("read from command (%d) at %x\n", cmd_latch, addr);
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static READ_HANDLER( latch2_r )
{
	i8039_set_irq_line(0, CLEAR_LINE);
	return i8039_latch;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },                 /* rom soundboard     */
	{ 0x6000, 0x63ff, MRA_RAM },                 /* ram soundboard     */
	{ 0x8000, 0x8000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },                 /* rom soundboard     */
	{ 0x6000, 0x63ff, MWA_RAM },                 /* ram soundboard     */
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
  	{ 0x05, 0x05, AY8910_read_port_1_r },
	{ 0x09, 0x09, AY8910_read_port_2_r },
  	{ 0x0d, 0x0d, AY8910_read_port_3_r },
  	{ 0x11, 0x11, AY8910_read_port_4_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x04, 0x04, AY8910_control_port_1_w },
	{ 0x06, 0x06, AY8910_write_port_1_w },
	{ 0x08, 0x08, AY8910_control_port_2_w },
	{ 0x0a, 0x0a, AY8910_write_port_2_w },
	{ 0x0c, 0x0c, AY8910_control_port_3_w },
	{ 0x0e, 0x0e, AY8910_write_port_3_w },
	{ 0x10, 0x10, AY8910_control_port_4_w },
	{ 0x12, 0x12, AY8910_write_port_4_w },
	{ 0x14, 0x14, gyruss_i8039_irq_w },
	{ 0x18, 0x18, latch2_w },
PORT_END

static MEMORY_READ_START( i8039_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( i8039_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
MEMORY_END

static PORT_READ_START( i8039_readport )
	{ 0x00, 0xff, latch2_r },
PORT_END

static PORT_WRITE_START( i8039_writeport )
	{ I8039_p1, I8039_p1, DAC_0_data_w },
	{ I8039_p2, I8039_p2, IOWP_NOP },
PORT_END

M1_BOARD_START( gyruss )
	MDRV_NAME( "Gyruss" )
	MDRV_HWDESC( "Z80, i8039, AY-3-8910(x5), DAC" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( GY_Init )
	MDRV_SEND( GY_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_PORTS(sound_readport, sound_writeport)

	MDRV_CPU_ADD(I8039, I8039_CLOCK)
	MDRV_CPU_MEMORY(i8039_readmem,i8039_writemem)
	MDRV_CPU_PORTS(i8039_readport,i8039_writeport)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static READ_HANDLER(gy_portA_r)
{
	static long timerdata[20] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x09, 0x0a, 0x0b, 0x0a, 0x0d };

	return timerdata[cpucycle%10];
}

static void gy_timer(int refcon)
{
 	cpucycle++;

	timer_set(TIMER_RATE, 0, gy_timer);
}

static void GY_Init(long srate)
{
	timer_set(TIMER_RATE, 0, gy_timer);
}

static void GY_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
