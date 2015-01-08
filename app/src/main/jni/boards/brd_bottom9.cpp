// Bottom of the Ninth: Z80 + 2xK007232
// S.P.Y. - Special Project Y: Z80 + YM3812 + 2xK007232

#include "m1snd.h"

static void B9_Init(long srate);
static void B9_SendCmd(int cmda, int cmdb);

static int cmd_latch, nmienable;

static void volume_callback0(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static void volume_callback1(int v)
{
	K007232_set_volume(1,0,(v >> 4) * 0x11,0);
	K007232_set_volume(1,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	2,			/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory regions */
	{ K007232_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER),
			K007232_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback0, volume_callback1 }	/* external port callback */
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static void bottom9_sound_interrupt(void)
{
	if (nmienable)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	}
}

static WRITE_HANDLER( nmi_enable_w )
{
	nmienable = data;
}

static WRITE_HANDLER( sound_bank_w )
{
	int bank_A,bank_B;

	bank_A = ((data >> 0) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 0, bank_A, bank_B );
	bank_A = ((data >> 4) & 0x03);
	bank_B = ((data >> 6) & 0x03);
	K007232_set_bank( 1, bank_A, bank_B );
}

static MEMORY_READ_START( bottom9_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa00d, K007232_read_port_0_r },
	{ 0xb000, 0xb00d, K007232_read_port_1_r },
	{ 0xd000, 0xd000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( bottom9_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, sound_bank_w },
	{ 0xa000, 0xa00d, K007232_write_port_0_w },
	{ 0xb000, 0xb00d, K007232_write_port_1_w },
	{ 0xf000, 0xf000, nmi_enable_w },
MEMORY_END

M1_BOARD_START( bottom9 )
	MDRV_NAME("Bottom of the Ninth")
	MDRV_HWDESC("Z80, K007232(x2)")
	MDRV_INIT( B9_Init )
	MDRV_SEND( B9_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(bottom9_sound_readmem,bottom9_sound_writemem)

	MDRV_SOUND_ADD(K007232, &k007232_interface)
M1_BOARD_END

static void timer(int refcon)
{
	bottom9_sound_interrupt();
	timer_set(1.0/480.0, 0, timer);
}

static void B9_Init(long srate)
{
	nmienable = 0;
	timer_set(1.0/480.0, 0, timer);
}

static void B9_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}


static MEMORY_READ_START( spy_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa00d, K007232_read_port_0_r },
	{ 0xb000, 0xb00d, K007232_read_port_1_r },
	{ 0xc000, 0xc000, YM3812_status_port_0_r },
	{ 0xd000, 0xd000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( spy_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, sound_bank_w },
	{ 0xa000, 0xa00d, K007232_write_port_0_w },
	{ 0xb000, 0xb00d, K007232_write_port_1_w },
	{ 0xc000, 0xc000, YM3812_control_port_0_w },
	{ 0xc001, 0xc001, YM3812_write_port_0_w },
MEMORY_END

static struct K007232_interface spy_k007232_interface =
{
	2,		/* number of chips */
	3579545,	/* clock */
	{ RGN_SAMP1, RGN_SAMP2 },	/* memory regions */
	{
		K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER),
		K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER),
	},
	{ volume_callback0, volume_callback1 },	/* external port callback */
};

static void ym3812_irq(int irq)
{
	if (irq)
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	else
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{																	
	1,
	3579545,
	{ 100 },
	{ ym3812_irq },
};				

M1_BOARD_START( spy )
	MDRV_NAME("S.P.Y. - Special Project Y")
	MDRV_HWDESC("Z80, YM3812, K007232(x2)")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( B9_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(spy_readmem_sound, spy_writemem_sound)

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(K007232, &spy_k007232_interface)
M1_BOARD_END
