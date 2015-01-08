/*
	Taito SJ System and	Halley's Comet/Ben Beroh Beh

	Notes:
	SJ also have an additional AY-8910, under control of the main 6809.
	SJ games have a DAC connected to one of the AY-8910	
*/
#include "m1snd.h"

static int cmd_latch, nmi_disable;

static WRITE8_HANDLER( nmi_disable_w )
{
	nmi_disable = data & 1;
}

static struct AY8910interface sj_ay8910_interface =
{
	3,			/* 3 chips */
	6000000/4,
	{ MIXER(15,MIXER_PAN_CENTER), MIXER(15,MIXER_PAN_CENTER), MIXER(15,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0, 0 },
	{ 0, 0, 0, nmi_disable_w }
};

static READ8_HANDLER( latch_r )
{
//	int PC = z80_get_reg(REG_PC);
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);

	return cmd_latch;
}

static void timer(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIME_IN_NSEC(27306667), 0, timer);
}

static void SJ_Init(long srate)
{
	timer_set(TIME_IN_NSEC(27306667), 0, timer);

	/* Halley's Comet and Ben Bero Beh need this */
	if (Machine->refcon == 1)
		m1snd_addToCmdQueue(0xef);
}

static void SJ_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	if (!nmi_disable)
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static MEMORY_READ_START( sj_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM }, /* HC/BBB have 1kB of additional RAM */
	{ 0x4801, 0x4801, AY8910_read_port_0_r },
	{ 0x4803, 0x4803, AY8910_read_port_1_r },
	{ 0x4805, 0x4805, AY8910_read_port_2_r },
	{ 0x5000, 0x5000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sj_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_0_w },
	{ 0x4801, 0x4801, AY8910_write_port_0_w },
	{ 0x4802, 0x4802, AY8910_control_port_1_w },
	{ 0x4803, 0x4803, AY8910_write_port_1_w },
	{ 0x4804, 0x4804, AY8910_control_port_2_w },
	{ 0x4805, 0x4805, AY8910_write_port_2_w },
	{ 0xe000, 0xefff, MWA_NOP },
MEMORY_END

M1_BOARD_START( taitosj )
	MDRV_NAME("Taito SJ System")
	MDRV_HWDESC("Z80, AY-3-8910(x3)")
	MDRV_INIT( SJ_Init )
	MDRV_SEND( SJ_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000/2)
	MDRV_CPU_MEMORY(sj_readmem,sj_writemem)

	MDRV_SOUND_ADD(AY8910, &sj_ay8910_interface)
M1_BOARD_END
