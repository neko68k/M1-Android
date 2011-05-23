/* Sega System 16 */

#include "m1snd.h"

#define YM_CLOCK (4000000)
#define Z80_CLOCK (8000000)

static void S16_Init(long srate);
static void S16_SendCmd(int cmda, int cmdb);
static void S16A_SendCmd(int cmda, int cmdb);
static void irq_7759(int parm);

static int cmd_latch;

static struct YM2151interface ym2151_interface = {
	1,			/* 1 chip */
	4000000,		/* 4.0 MHz, confirmed on schematics */
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct UPD7759_interface upd7759_interface =
{
	1,			/* 1 chip */
	{ 60 }, 	/* volumes */
	{ RGN_CPU2 },			/* memory region 3 contains the sample data */
	UPD7759_SLAVE_MODE,
	{ irq_7759 },
};

static unsigned int port_8255_c03 = 0;
static unsigned int port_8255_c47 = 0;
static unsigned int port_7751_p27 = 0;
static unsigned int rom_offset = 0;
static unsigned int rom_base = 0;
static unsigned int rom_bank = 0;
static int dac_enable = 0;

static void trigger_7751_sound(int data)
{
	/* I think this is correct for 128k sound roms,
	     it's OK for smaller roms */
	if((data&0xf) == 0xc) rom_bank=0;
	else if((data&0xf) == 0xd) rom_bank=0x4000;
	else if((data&0xf) == 0xb) rom_bank=0xc000;
	else if((data&0xf) == 0xa) rom_bank=0x8000;

	else if((data&0xf) == 0xf) rom_bank=0x1c000;
	else if((data&0xf) == 0xe) rom_bank=0x18000;
	else if((data&0xf) == 0x7) rom_bank=0x14000;
	else if((data&0xf) == 0x6) rom_bank=0x10000;

	port_8255_c03 = (data>>5);

//	cpu_set_irq_line(2, 0, PULSE_LINE);
	n7751_set_irq_line(0, ASSERT_LINE);
	n7751_set_irq_line(0, CLEAR_LINE);
}

// I'm sure this must be wrong, but it seems to work for quartet music.
WRITE_HANDLER( sys16_7751_audio_8255_w )
{
	logerror((char *)"7751: %4x %4x\n",data,data^0xff);

	if ((data & 0x0f) != 8)
	{
		n7751_reset(NULL);
		timer_set(TIME_IN_USEC(300), data, trigger_7751_sound);
	}
}


READ_HANDLER( sys16_7751_audio_8255_r )
{
	// Only PC4 is hooked up
	/* 0x00 = BUSY, 0x10 = NOT BUSY */
	return (port_8255_c47 & 0x10);
}

/* read from BUS */
READ_HANDLER( sys16_7751_sh_rom_r )
{
	unsigned char *sound_rom = memory_region(REGION_SOUND1);

	return sound_rom[rom_offset+rom_base];
}

/* read from T1 */
READ_HANDLER( sys16_7751_sh_t1_r )
{
	// Labelled as "TEST", connected to ground
	return 0;
}

/* read from P2 */
READ_HANDLER( sys16_7751_sh_command_r )
{
	// 8255's PC0-2 connects to 7751's S0-2 (P24-P26 on an 8048)
	return ((port_8255_c03 & 0x07) << 4) | port_7751_p27;
}

/* write to P1 */
WRITE_HANDLER( sys16_7751_sh_dac_w )
{
	if (!dac_enable) return;
	DAC_data_w(0,data);
}

/* write to P2 */
WRITE_HANDLER( sys16_7751_sh_busy_w )
{
	port_8255_c03 = (data & 0x70) >> 4;
	port_8255_c47 = (data & 0x80) >> 3;
	port_7751_p27 = data & 0x80;
	rom_base = rom_bank;
}

/* write to P4 */
WRITE_HANDLER( sys16_7751_sh_offset_a0_a3_w )
{
	rom_offset = (rom_offset & 0xFFF0) | (data & 0x0F);
}

/* write to P5 */
WRITE_HANDLER( sys16_7751_sh_offset_a4_a7_w )
{
	rom_offset = (rom_offset & 0xFF0F) | ((data & 0x0F) << 4);
}

/* write to P6 */
WRITE_HANDLER( sys16_7751_sh_offset_a8_a11_w )
{
	rom_offset = (rom_offset & 0xF0FF) | ((data & 0x0F) << 8);
}

/* write to P7 */
WRITE_HANDLER( sys16_7751_sh_rom_select_w )
{
	rom_offset = (rom_offset & 0x0FFF) | ((0x4000 + ((data&0xf) << 12)) & 0x3000);

}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static READ_HANDLER( latch_16ar )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( UPD7759_bank_w )
{
	int size = (memory_region_length(REGION_CPU1)-0x10000);
	int mask = size <= 0x20000 ? 0x1ffff : size < 0x40000 ? 0x3ffff : 0x7ffff;
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000 + ((0x4000*data) & mask));

	UPD7759_reset_w(0, data & 0x40);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_BANK1 },
	{ 0xe800, 0xe800, latch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, latch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, UPD7759_bank_w },
	{ 0x80, 0x80, UPD7759_0_port_w },
PORT_END

static MEMORY_READ_START( sound_16areadmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe800, 0xe800, latch_16ar },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_16awritemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_16areadport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, latch_16ar },
PORT_END

static PORT_WRITE_START( sound_16awriteport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x80, sys16_7751_audio_8255_w },
PORT_END

static PORT_WRITE_START( sound_16ano7751writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

static MEMORY_READ_START( readmem_7751 )
	{ 0x0000, 0x03ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_7751 )
	{ 0x0000, 0x03ff, MWA_ROM },
MEMORY_END

static PORT_READ_START( readport_7751 )
	{ I8039_t1,  I8039_t1,  sys16_7751_sh_t1_r },
	{ I8039_p2,  I8039_p2,  sys16_7751_sh_command_r },
	{ I8039_bus, I8039_bus, sys16_7751_sh_rom_r },
PORT_END

static PORT_WRITE_START( writeport_7751 )
	{ I8039_p1, I8039_p1, sys16_7751_sh_dac_w },
	{ I8039_p2, I8039_p2, sys16_7751_sh_busy_w },
	{ I8039_p4, I8039_p4, sys16_7751_sh_offset_a0_a3_w },
	{ I8039_p5, I8039_p5, sys16_7751_sh_offset_a4_a7_w },
	{ I8039_p6, I8039_p6, sys16_7751_sh_offset_a8_a11_w },
	{ I8039_p7, I8039_p7, sys16_7751_sh_rom_select_w },
PORT_END

M1_BOARD_START( system16 )
	MDRV_NAME("System 16 (sound type 3)")
	MDRV_HWDESC("Z80, YM2151, uPD7759")
	MDRV_DELAYS( 60, 60 )
	MDRV_INIT( S16_Init )
	MDRV_SEND( S16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END

struct DACinterface sys16_7751_dac_interface =
{
	1,
	{ 40 }
};

M1_BOARD_START( system16a )
	MDRV_NAME("System 16 (sound type 2)")
	MDRV_HWDESC("Z80, uPD7751, YM2151, DAC")
	MDRV_DELAYS( 1000, 60 )
	MDRV_INIT( S16_Init )
	MDRV_SEND( S16A_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_16areadmem,sound_16awritemem)
	MDRV_CPU_PORTS(sound_16areadport,sound_16awriteport)

	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_MEMORY(readmem_7751,writemem_7751)
	MDRV_CPU_PORTS(readport_7751,writeport_7751)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(DAC, &sys16_7751_dac_interface)
M1_BOARD_END

M1_BOARD_START( system16ano7751 )
	MDRV_NAME("System 16 (sound type 1)")
	MDRV_HWDESC("Z80, YM2151")
	MDRV_DELAYS( 60, 60 )
	MDRV_INIT( S16_Init )
	MDRV_SEND( S16A_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_16areadmem,sound_16awritemem)
	MDRV_CPU_PORTS(sound_16areadport,sound_16ano7751writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END

static void irq_7759(int parm)
{
	if (Machine->refcon == 1)
	{
		return;
	}

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
}

static void S16_Init(long srate)
{
	dac_enable = 0;

	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000);
}

static void S16_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	dac_enable = 1;

	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static void S16A_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	dac_enable = 1;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

