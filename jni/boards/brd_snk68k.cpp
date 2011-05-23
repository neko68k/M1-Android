/* SNK 68000-based games with Z80, YM3812 and upd7759 */
#include "m1snd.h"

#define	AUDIO_CLOCK		4000000

static void SNK68K_Init(long foo);
static void SNK68K_SendCmd(int cmda, int cmdb);

static UINT8 cmd_latch = 0;


static READ_HANDLER( sound_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( D7759_write_port_0_w )
{
	UPD7759_port_w(offset, data);
	UPD7759_start_w (0, 0);
	UPD7759_start_w (0, 1);
}


static MEMORY_READ_START( snk68k_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, sound_r },
MEMORY_END

static MEMORY_WRITE_START( snk68k_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snk68k_sound_readport )
	{ 0x00, 0x00, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( snk68k_sound_writeport )
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x20, 0x20, YM3812_write_port_0_w },
	{ 0x40, 0x40, D7759_write_port_0_w },
	{ 0x80, 0x80, UPD7759_0_reset_w },
PORT_END

static void YM3812IRQ(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct UPD7759_interface upd7759_interface =
{
	1,
	{ 50 },
	{ RGN_SAMP1 },
	UPD7759_STANDALONE_MODE,
	{ 0 },
};

static struct YM3812interface ym3812_interface =
{
	1,
	AUDIO_CLOCK,
	{ 50 },
	{ YM3812IRQ },
};


M1_BOARD_START( snk68k )
	MDRV_NAME("SNK 68000 Hardware")
	MDRV_HWDESC("Z80, YM3812, uPD7759")
	MDRV_INIT( SNK68K_Init )
	MDRV_SEND( SNK68K_SendCmd )

	MDRV_CPU_ADD(Z80C, AUDIO_CLOCK)
	MDRV_CPU_MEMORY(snk68k_sound_readmem, snk68k_sound_writemem)
	MDRV_CPU_PORTS(snk68k_sound_readport, snk68k_sound_writeport)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END


static void SNK68K_Init(long foo)
{
	m1snd_addToCmdQueue(0x01);
	m1snd_addToCmdQueue(0x01);
	m1snd_addToCmdQueue(0x0d);
	m1snd_addToCmdQueue(0x0d);
}


static void SNK68K_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

