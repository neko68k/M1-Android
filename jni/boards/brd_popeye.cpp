/* Nintendo Popeye */

#include "m1snd.h"

static void Popeye_Init(long srate);
static void Popeye_SendCmd(int cmda, int cmdb);

#define TIMER_RATE (1.0/60.0)	// VBL

static struct AY8910interface popeye_ay8910_interface =
{
	1,			/* 2 chips */
	2000000,	/* 2 MHz ?? */
	{ MIXER(66,MIXER_PAN_LEFT), MIXER(66,MIXER_PAN_RIGHT) },	/* dotron clips with anything higher */
	{ 0 },
	{ 0 },
};

/* the protection device simply returns the last two values written shifted left */
/* by a variable amount. */
static int prot0,prot1,prot_shift;

static READ8_HANDLER( protection_r )
{
	if (offset == 0)
	{
		return ((prot1 << prot_shift) | (prot0 >> (8-prot_shift))) & 0xff;
	}
	else	/* offset == 1 */
	{
		/* the game just checks if bit 2 is clear. Returning 0 seems to be enough. */
		return 0;
	}
}

static WRITE8_HANDLER( protection_w )
{
	if (offset == 0)
	{
		/* this is the same as the level number (1-3) */
		prot_shift = data & 0x07;
	}
	else	/* offset == 1 */
	{
		prot0 = prot1;
		prot1 = data;
	}
}

static READ8_HANDLER(inputs_r)
{
	printf("Read input %x\n", offset);
	if (offset == 1) return 0x40;

	return 0x00;
}

static MEMORY_READ_START( popeye_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xe000, 0xe001, protection_r },
MEMORY_END

static MEMORY_WRITE_START( popeye_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0xe000, 0xe001, protection_w },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0x00, 0x02, inputs_r },
	{ 0x03, 0x03, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
PORT_END

M1_BOARD_START( popeye )
	MDRV_NAME("Popeye")
	MDRV_HWDESC("Z80, AY-3-8910")
	MDRV_DELAYS( 400, 40 )
	MDRV_INIT( Popeye_Init )
	MDRV_SEND( Popeye_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(popeye_readmem,popeye_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(AY8910, &popeye_ay8910_interface)
M1_BOARD_END

static void timer(int ref)
{
	if (z80_get_reg(Z80_I) & 1)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	}

	timer_set(TIMER_RATE, 0, timer);
}

static void Popeye_Init(long srate)
{
	int i;

	timer_set(TIMER_RATE, 0, timer);
}

static void Popeye_SendCmd(int cmda, int cmdb)
{
}
