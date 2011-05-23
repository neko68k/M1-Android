/* Alpha Denshi 'Equites' Sound Board */
#include <math.h>
#include "m1snd.h"

static void Equites_Init(long srate);
static void Equites_SendCmd(int cmda, int cmdb);

static int cmd_latch;
//static void *periodic_timer;
static void *synth_timer;


static WRITE_HANDLER(equites_5232_w)
{
	/* Gets around a current 5232 emulation restriction */
	if (offset < 0x08 && data)
		data |= 0x80;

	MSM5232_0_w(offset, data);
}

static WRITE_HANDLER(equites_8910control_w)
{
	AY8910Write(0, 0, data);
}

static WRITE_HANDLER(equites_8910data_w)
{
	AY8910Write(0, 1, data);	
}

static WRITE_HANDLER(equites_dac0_w)
{
	DAC_signed_data_w(0, data << 2);
}

static WRITE_HANDLER(equites_dac1_w)
{
	DAC_signed_data_w(1, data << 2);
}

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static WRITE_HANDLER( latch_clear_w )
{
	cmd_latch = 0;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM }, // sound program
	{ 0xc000, 0xc000, latch_r },
	{ 0xe000, 0xe0ff, MRA_RAM }, // stack and variables
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM }, // sound program
	{ 0xc080, 0xc08d, equites_5232_w },
	{ 0xc0a0, 0xc0a0, equites_8910data_w },
	{ 0xc0a1, 0xc0a1, equites_8910control_w },
	{ 0xc0b0, 0xc0b0, MWA_NOP }, // INTR: sync with main melody
	{ 0xc0c0, 0xc0c0, MWA_NOP }, // INTR: sync with specific beats
	{ 0xc0d0, 0xc0d0, equites_dac0_w },
	{ 0xc0e0, 0xc0e0, equites_dac1_w },
	{ 0xc0f8, 0xc0fe, MWA_NOP }, // soundboard I/O, ignored
	{ 0xc0ff, 0xc0ff, latch_clear_w },
	{ 0xe000, 0xe0ff, MWA_RAM }, // stack and variables
MEMORY_END

static struct MSM5232interface msm5232_interface =
{
	1,
	2500000,
	{ { 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6 } },
	{ MIXER(75, MIXER_PAN_CENTER) }
};

static struct AY8910interface ay8910_interface =
{
	1,
	6144444/4,
	{ MIXER(50, MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },	/* equites_8910porta_w */
	{ 0 },	/* equites_8910portb_w */
	{ 0 }
};

static struct DACinterface dac_interface =
{
	2,
	{ 75, 75 }
};

M1_BOARD_START( equites )
	MDRV_NAME("Equites Sound Board")
	MDRV_HWDESC("8085A, MSM5232, AY-3-8910, DAC(x2)")
	MDRV_INIT( Equites_Init )
	MDRV_SEND( Equites_SendCmd )

	MDRV_CPU_ADD(8085A, 5000000)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, &msm5232_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static void synth_callback(int ref)
{
	static int parity = 0;

	if (parity^=1)
	{
		cpu_set_irq_line(0, I8085_INTR_LINE, ASSERT_LINE);
		cpu_set_irq_line(0, I8085_INTR_LINE, CLEAR_LINE);
	}

	cpu_set_irq_line(0, I8085_RST75_LINE, ASSERT_LINE);
	cpu_set_irq_line(0, I8085_RST75_LINE, CLEAR_LINE);
}


static void Equites_Init(long srate)
{
	synth_timer = timer_pulse(TIME_IN_HZ(106), 0, synth_callback);

	m1snd_addToCmdQueueRaw(0xf0);
}

static void Equites_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
