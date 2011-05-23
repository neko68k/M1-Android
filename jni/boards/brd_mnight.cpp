/* UPL "Mutant Night" */

#include "m1snd.h"

static int cmd_latch;

static void MN_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport )
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
PORT_END


static MEMORY_READ_START( snd_readmem_2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xc000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem_2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport_2 )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( snd_writeport_2 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,	 /* 2 chips */
	12000000/8, // lax 11/03/1999  (1250000 -> 1500000 ???)
	{ YM2203_VOL(50,15), YM2203_VOL(50,15)},
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
};

static struct YM2203interface ym2203_interface_2 =
{
	1,
	6000000 / 4,
	{ YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
};

M1_BOARD_START( mnight )
	MDRV_NAME("Mutant Night")			// hardware name
	MDRV_HWDESC("Z80, YM2203(x2)")	// hardware description
	MDRV_SEND( MN_SendCmd )		// routine to send a command to the hardware

	MDRV_CPU_ADD(Z80C, 5000000)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

M1_BOARD_START( argus )
	MDRV_NAME("Argus")			// hardware name
	MDRV_HWDESC("Z80, YM2203")	// hardware description
	MDRV_SEND( MN_SendCmd )		// routine to send a command to the hardware

	MDRV_CPU_ADD(Z80C, 5000000)
	MDRV_CPU_MEMORY(snd_readmem_2,snd_writemem_2)
	MDRV_CPU_PORTS(snd_readport_2,snd_writeport_2)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface_2)
M1_BOARD_END

M1_BOARD_START( valtric )
	MDRV_NAME("Valtric")			// hardware name
	MDRV_HWDESC("Z80, YM2203(x2)")	// hardware description
	MDRV_SEND( MN_SendCmd )		// routine to send a command to the hardware

	MDRV_CPU_ADD(Z80C, 5000000)
	MDRV_CPU_MEMORY(snd_readmem_2,snd_writemem_2)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END



