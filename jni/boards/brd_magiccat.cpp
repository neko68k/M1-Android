/*

 Face Co. "LINDA" board series
 Magical Cat Adventure + Nostradamus 
 Z80 + YM2610 

Magical Cat:
0000-3FFF: fixed ROM
4000-BFFF: banked ROM
C000-DFFF: work RAM
E000-E003: YM2610
F000     : bankswitch.  map ((start of ROM) + (value written at f000 * 0x4000)) in
           the bank window at 0x4000.

port 0x80 = latch

Nostradamus:
0000-7FFF: fixed ROM
8000-BFFF: banked ROM
C000-DFFF: work RAM
E000-FFFF: unknown

ports:
0-3: YM2610 write
4-7: YM2610 read
0x40: bankswitch
0x80: latch

IRQ = YM2610
NMI = 68k wrote to latch

*/

#include "m1snd.h"

#define Z80_CLOCK (4000000)

static void MCat_Init(long srate);
static void Nost_SendCmd(int cmda, int cmdb);
static void MCat_SendCmd(int cmda, int cmdb);
static void YM_IRQ(int irq);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ MIXERG(8,MIXER_GAIN_4x,MIXER_PAN_CENTER) }, // volume of AY-3-8910 voices
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQ },
	{ RGN_SAMP1 },
	{ RGN_SAMP1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static WRITE_HANDLER ( mcatadv_sound_bw_w )
{
	data8_t *rom = memory_region(REGION_CPU1);

//	printf("bank %x\n", data);

	cpu_setbank(1,rom + data * 0x4000);
}


static MEMORY_READ_START( mcatadv_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM		},	// ROM
	{ 0x4000, 0xbfff, MRA_BANK1		},	// ROM
	{ 0xc000, 0xdfff, MRA_RAM		},	// RAM
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r		},
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r		},
MEMORY_END

static MEMORY_WRITE_START( mcatadv_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM		},	// ROM
	{ 0x4000, 0xbfff, MWA_ROM		},	// ROM
	{ 0xc000, 0xdfff, MWA_RAM		},	// RAM
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xf000, 0xf000, mcatadv_sound_bw_w },
MEMORY_END

static PORT_READ_START( mcatadv_sound_readport )
	{ 0x80, 0x80, latch_r },
PORT_END

static PORT_WRITE_START( mcatadv_sound_writeport )
	{ 0x80, 0x80, IOWP_NOP },
PORT_END


static MEMORY_READ_START( nost_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM		},	// ROM
	{ 0x8000, 0xbfff, MRA_BANK1		},	// ROM
	{ 0xc000, 0xdfff, MRA_RAM		},	// RAM
MEMORY_END

static MEMORY_WRITE_START( nost_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM		},	// ROM
	{ 0x8000, 0xbfff, MWA_ROM		},	// ROM
	{ 0xc000, 0xdfff, MWA_RAM		},	// RAM
MEMORY_END

static PORT_READ_START( nost_sound_readport )
	{ 0x04, 0x05, YM2610_status_port_0_A_r },
	{ 0x06, 0x07, YM2610_status_port_0_B_r },
	{ 0x80, 0x80, latch_r },
PORT_END

static PORT_WRITE_START( nost_sound_writeport )
	{ 0x00, 0x00, YM2610_control_port_0_A_w },
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w },
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0x40, 0x40, mcatadv_sound_bw_w },
	{ 0x80, 0x80, IOWP_NOP },
PORT_END

M1_BOARD_START( magiccat )
	MDRV_NAME("Face LINDA5")
	MDRV_HWDESC("Z80, YM2610")
	MDRV_DELAYS( 100, 50 )
	MDRV_INIT( MCat_Init )
	MDRV_SEND( MCat_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(mcatadv_sound_readmem,mcatadv_sound_writemem)
	MDRV_CPU_PORTS(mcatadv_sound_readport,mcatadv_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &ym2610_interface)
M1_BOARD_END

M1_BOARD_START( nost )
	MDRV_NAME("Face LINDA25")
	MDRV_HWDESC("Z80, YM2610")
	MDRV_DELAYS( 100, 50 )
	MDRV_SEND( Nost_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(nost_sound_readmem,nost_sound_writemem)
	MDRV_CPU_PORTS(nost_sound_readport,nost_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &ym2610_interface)
M1_BOARD_END

static void YM_IRQ(int irq)
{
//	printf("IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void MCat_Init(long srate)
{
	m1snd_addToCmdQueue(239);	// enabler
}

static void MCat_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void Nost_SendCmd(int cmda, int cmdb)
{
	// many commands can crash the z80!  (if the command # is less than or equal
	// to 16, it jumps through a table which only has 10 valid pointers!)
	cmda &= 0x7f;
	if (cmda <= 16 && cmda > 1) cmda = 0;
	if (cmda >= 36 && cmda <= 60) cmda = 0;
	if (cmda >= 114) cmda = 0;

	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

