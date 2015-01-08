/* Komax "Flower" (distributed by Sega) */
/* Z80 + custom wavetable chip(x2) */

#include "m1snd.h"

#define Flw_TIMER_PERIOD (1.0/90.0)

static void Flw_Init(long srate);
static void Flw_SendCmd(int cmda, int cmdb);

static int nmimask = 0, irqmask = 0, cmd_latch;

extern "C" {
extern data8_t *flower_soundregs1,*flower_soundregs2;
int flower_sh_start(const struct MachineSound *msound);
void flower_sh_stop(void);
WRITE_HANDLER( flower_sound1_w );
WRITE_HANDLER( flower_sound2_w );
};

static struct CustomSound_interface custom_interface =
{
	flower_sh_start,
	flower_sh_stop,
	0
};

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static WRITE_HANDLER( sn_irq_enable_w )
{
	irqmask = data;
}

static WRITE_HANDLER( sn_nmi_enable_w )
{
	if (!data)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	}

	nmimask = data;
}

static MEMORY_READ_START( flower_sn_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6000, 0x6000, latch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( flower_sn_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, sn_irq_enable_w },
	{ 0x4001, 0x4001, sn_nmi_enable_w },
	{ 0x8000, 0x803f, flower_sound1_w, &flower_soundregs1 },
	{ 0xa000, 0xa03f, flower_sound2_w, &flower_soundregs2 },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

M1_BOARD_START( flower )
	MDRV_NAME("Flower")
	MDRV_HWDESC("Z80, custom wavetable(x2)")
	MDRV_INIT( Flw_Init )
	MDRV_SEND( Flw_SendCmd )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(flower_sn_readmem, flower_sn_writemem)

	MDRV_SOUND_ADD(CUSTOM, &custom_interface)
M1_BOARD_END

static void flw_timer(int refcon)
{
	if (irqmask)
	{
		cpu_set_irq_line(0, 0, ASSERT_LINE);
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}

	// set up for next time
	timer_set(Flw_TIMER_PERIOD, 0, flw_timer);
}

static void Flw_Init(long srate)
{
	nmimask = 0;
	timer_set(Flw_TIMER_PERIOD, 0, flw_timer);

//	flower_soundregs1 = &workram[0x8000];
//	flower_soundregs2 = &workram[0xa000];
}

static void Flw_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	if (nmimask)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	}
}
