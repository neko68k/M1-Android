/* Tatsumi Buggy Boy and Lock-On */

#include "m1snd.h"

#define BB_Z80_CLK	3750000
#define LO_Z80_CLK	4000000

static void BuggyBoy_Init(long foo);
static void BuggyBoy_SendCmd(int cmda, int cmdb);
static void LockOn_SendCmd(int cmda, int cmdb);

static void *periodic_timer;

static struct AY8910interface ay8910_interface =
{
	2,
	BB_Z80_CLK / 2,
	{ MIXER(10, MIXER_PAN_LEFT), MIXER(10, MIXER_PAN_RIGHT) },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static void ym2203_irq(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,
	LO_Z80_CLK,
	{ YM2203_VOL(MIXER(60, MIXER_PAN_CENTER), MIXER(30, MIXER_PAN_CENTER)) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ ym2203_irq },
};

/* Buggy Boy's RAM is shared with main 8086 */
static READ_HANDLER( bb_ram_r )
{
	int PC = z80_get_reg(REG_PC);

	/* Fake response from host to pass boot test */
	if (offset == 0x24)
	{
		switch (PC)
		{
			case 0xbb7: workram[offset] = 2; break;
			case 0xbc3: workram[offset] = 4; break;
			case 0xbd2: workram[offset] = 6; break;
		}
	}

	return workram[offset];
}

static WRITE_HANDLER( bb_ram_w )
{
	workram[offset] = data;
}

/* Z80 acknowledges interrupts itself - HOLD_LINE ought to do... */
static void irq_off(int ref)
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void timer_callback(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	timer_set(TIME_IN_NSEC(300), 0, irq_off);
}

static WRITE_HANDLER( irq_req_w )
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	timer_set(TIME_IN_NSEC(300), 0, irq_off);
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, bb_ram_r },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, bb_ram_w },
	{ 0x7000, 0x7000, irq_req_w },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ 0x80, 0x80, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ 0x41, 0x41, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_1_w },
	{ 0x81, 0x81, AY8910_control_port_1_w },
PORT_END


M1_BOARD_START( buggyboy )
	MDRV_NAME("Buggy Boy/Speed Buggy")
	MDRV_HWDESC("Z80, YM2149(x2)")
	MDRV_SEND( BuggyBoy_SendCmd )
	MDRV_INIT( BuggyBoy_Init )
	MDRV_CPU_ADD(Z80C, BB_Z80_CLK)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END

static void BuggyBoy_Init(long foo)
{
	periodic_timer = timer_pulse(TIME_IN_HZ(BB_Z80_CLK / 4 / 2048), 0, timer_callback);
}

static void delayed_w(int data)
{
	workram[9] = 0;
}

static void BuggyBoy_SendCmd(int cmda, int cmdb)
{
	workram[6] = cmda;
	workram[9] = 0x01;
	timer_set(TIME_IN_SEC(1.0/54.0), cmda, delayed_w);
}


/* Lock-On RAM is shared with the main V30 */
static READ_HANDLER( lo_ram_r )
{
	int PC = z80_get_reg(REG_PC);

	/* Fake response from host to pass boot test */
	if (offset == 0x10)
	{
		switch (PC)
		{
			case 0xa4c: workram[offset] = 2; break;
			case 0xa58: workram[offset] = 4; break;
			case 0xa67: workram[offset] = 6; break;
		}
	}

	return workram[offset];
}

static WRITE_HANDLER( lo_ram_w )
{
	workram[offset] = data;
}

static MEMORY_READ_START( lo_snd_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x7000, 0x7000, MRA_NOP },
	{ 0x7400, 0x7403, MRA_NOP },
	{ 0x7800, 0x7fff, lo_ram_r },
MEMORY_END

static MEMORY_WRITE_START( lo_snd_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x7000, 0x7000, MWA_NOP },
	{ 0x7400, 0x7403, MWA_NOP },
	{ 0x7800, 0x7fff, lo_ram_w },
MEMORY_END

static PORT_READ_START( lo_snd_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( lo_snd_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

M1_BOARD_START( lockon )
	MDRV_NAME("Lock-On")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_SEND( LockOn_SendCmd )
	MDRV_CPU_ADD(Z80C, LO_Z80_CLK)
	MDRV_CPU_MEMORY(lo_snd_readmem, lo_snd_writemem)
	MDRV_CPU_PORTS(lo_snd_readport, lo_snd_writeport)
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static void lo_delayed_w(int data)
{
	workram[9] = 0;
}

static void LockOn_SendCmd(int cmda, int cmdb)
{
	workram[9] = cmda;
	timer_set(TIME_IN_SEC(1.0/58.0), cmda, lo_delayed_w);
}
