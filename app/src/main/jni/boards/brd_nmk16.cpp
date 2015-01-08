// NMK 16-bit games

#include "m1snd.h"

static void NMK2_Init(long srate);
static void NMK_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static void irq_handler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static struct YM2203interface ym2203_interface_15 =
{
	1,			/* 1 chip */
	1500000,	/* 2 MHz ??? */
	{ YM2203_VOL(99,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irq_handler }
};

static struct OKIM6295interface okim6295_interface_dual =
{
	2,              					/* 2 chips */
	{ 16000000/4/165, 16000000/4/165 },	/* 24242Hz frequency? */
	{ RGN_SAMP1, RGN_SAMP2 },	/* memory region */
	{ 6, 6 }							/* volume */
};

static MEMORY_READ_START( manybloc_sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xf000, 0xf000, latch_r },
	{ 0xf400, 0xf400, OKIM6295_status_0_r },
	{ 0xf500, 0xf500, OKIM6295_status_1_r },
MEMORY_END

static MEMORY_WRITE_START( manybloc_sound_writemem)
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf400, 0xf400, OKIM6295_data_0_w },
	{ 0xf500, 0xf500, OKIM6295_data_1_w },
	{ 0xf600, 0xf600, MWA_NOP },
	{ 0xf700, 0xf700, MWA_NOP },
MEMORY_END

static PORT_READ_START( manybloc_sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( manybloc_sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

M1_BOARD_START( manybloc )
	MDRV_NAME("Many Block")
	MDRV_HWDESC("Z80, YM2203, MSM-6295(x2)")
	MDRV_SEND( NMK_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(manybloc_sound_readmem,manybloc_sound_writemem)
	MDRV_CPU_PORTS(manybloc_sound_readport,manybloc_sound_writeport)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface_15)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface_dual)
M1_BOARD_END

static WRITE_HANDLER( macross2_sound_bank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	cpu_setbank(1, rom + (data & 0x07) * 0x4000);
}

static WRITE_HANDLER( macross2_oki6295_bankswitch_w )
{
	/* The OKI6295 ROM space is divided in four banks, each one indepentently
	   controlled. The sample table at the beginning of the addressing space is
	   divided in four pages as well, banked together with the sample data. */
	#define TABLESIZE 0x100
	#define BANKSIZE 0x10000
	int chip = (offset & 4) >> 2;
	int banknum = offset & 3;
	unsigned char *rom = memory_region(REGION_SOUND1 + chip);
	int size = memory_region_length(REGION_SOUND1 + chip) - 0x40000;
	int bankaddr = (data * BANKSIZE) & (size-1);

	/* copy the samples */
	memcpy(rom + banknum * BANKSIZE,rom + 0x40000 + bankaddr,BANKSIZE);

	/* and also copy the samples address table */
	rom += banknum * TABLESIZE;
	memcpy(rom,rom + 0x40000 + bankaddr,TABLESIZE);
}

static MEMORY_READ_START( macross2_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },	/* banked ROM */
	{ 0xa000, 0xa000, MRA_NOP },	/* IRQ ack? watchdog? */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xf000, 0xf000, latch_r },	/* from 68000 */
MEMORY_END

static MEMORY_WRITE_START( macross2_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe001, 0xe001, macross2_sound_bank_w },
	{ 0xf000, 0xf000, MWA_NOP },	/* to 68000 */
MEMORY_END

static PORT_READ_START( macross2_sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x80, 0x80, OKIM6295_status_0_r },
	{ 0x88, 0x88, OKIM6295_status_1_r },
PORT_END

static PORT_WRITE_START( macross2_sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, OKIM6295_data_0_w },
	{ 0x88, 0x88, OKIM6295_data_1_w },
	{ 0x90, 0x97, macross2_oki6295_bankswitch_w },
PORT_END

M1_BOARD_START( macross2 )
	MDRV_NAME("Macross 2")
	MDRV_HWDESC("Z80, YM2203, MSM-6295(x2)")
	MDRV_DELAYS( 500, 150 )
	MDRV_SEND( NMK_SendCmd )
	MDRV_INIT( NMK2_Init )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(macross2_sound_readmem,macross2_sound_writemem)
	MDRV_CPU_PORTS(macross2_sound_readport,macross2_sound_writeport)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface_15)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface_dual)
M1_BOARD_END

static void NMK2_Init(long srate)
{
	cmd_latch = 0xff;

	m1snd_addToCmdQueue(1);
	m1snd_addToCmdQueue(0);
	m1snd_addToCmdQueue(0xffff);
}

static void NMK_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xffff)
	{
		m1snd_initNormalizeState();
	}

	cmd_latch = cmda;
}
