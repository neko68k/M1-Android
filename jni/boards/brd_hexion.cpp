/*
** Hexion, from Konami
** M1 driver by unknownfile, 2009
** Ripped off hexion.c by N. Salmoria
**
** Driver uses RAM space at 0xad00 for the replayer
** Write to 0xafcf to play musix
** Sound is updated on NMIs
**
*/

#include "m1snd.h"

static int hexion_rombank=4;
//static int hexion_rambank=0;

static unsigned char hex_mem[0x2000];

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 1056000 / 132 },
	{ REGION_SOUND1 },
	{ 52 }
};

static struct k051649_interface k051649_interface =
{
	24000000/16,	/* not real clock! */
	30,			/* Volume */
};

static READ_HANDLER (hex_mem_r) {
	return hex_mem[offset & 0x1fff];
}

static WRITE_HANDLER (hex_mem_w) {
	hex_mem[offset & 0x1fff] = data;
}

static WRITE_HANDLER ( hexion_bankswitch_w ) {
	hexion_rombank = (data & 0x0f);
//	printf("ROM bankswitch to %02X\n",hexion_rombank);
}

static READ_HANDLER ( hexion_bankedrom_r ) {
	//printf("banked rom read at logical offset %08X, pc = %04X\n",offset,z80_get_reg(1));
	return prgrom[ (hexion_rombank * 0x2000) + offset ];
}

static READ_HANDLER ( watchdog_r ) {
	return 0xff;
}

static MEMORY_READ_START ( hexion_readmem ) 
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, hexion_bankedrom_r },
	{ 0xa000, 0xbfff, hex_mem_r },
	{ 0xf540, 0xf540, watchdog_r },
MEMORY_END

static MEMORY_WRITE_START ( hexion_writemem ) 
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xbfff, hex_mem_w },
	{ 0xe800, 0xe87f, K051649_waveform_w },
	{ 0xe880, 0xe889, K051649_frequency_w },
	{ 0xe88a, 0xe88e, K051649_volume_w },
	{ 0xe88f, 0xe88f, K051649_keyonoff_w },
	{ 0xf200, 0xf200, OKIM6295_data_0_w },
	{ 0xf480, 0xf480, hexion_bankswitch_w },
MEMORY_END

void Hexion_Vblank_Clear (int refcon);

void Hexion_Vblank (int refcon)
{
	cpu_set_irq_line(0, 0, PULSE_LINE); // uses NMI
	timer_set((1.0/120.0)/262.0,0,Hexion_Vblank_Clear);
}

void Hexion_Vblank_Clear (int refcon) {
	cpu_set_irq_line(0, 0, CLEAR_LINE); // uses NMI
	timer_set(1.0/120.0, 0, Hexion_Vblank);
}


static void Hexion_Init (long and_winding_code ) {
	prgrom[0x3e6] = prgrom[0x3e7] = 0x00;
//	prgrom[0x400] = prgrom[0x401] = 0x00;
//	prgrom[0x4b6] = prgrom[0x4b7] = 0;

	// loader stub
	prgrom[0x00] = 0;
	prgrom[0x06] = 0x3e;
	prgrom[0x07] = 0x07;
	prgrom[0x08] = 0x32;
	prgrom[0x09] = 0xda;
	prgrom[0x0a] = 0xaf;
	prgrom[0x0b] = 0x00;
	prgrom[0x0c] = 0x3e;
	prgrom[0x0d] = 0x80;
	prgrom[0x0e] = 0x00;
	prgrom[0x0f] = 0x32;
	prgrom[0x10] = 0xcf;
	prgrom[0x11] = 0xaf;
	prgrom[0x12] = 0xfb;
	prgrom[0x13] = 0x18;
	prgrom[0x14] = 0xfe;

	prgrom[0x3a] = 0x66;
	prgrom[0x3b] = 0x00;

	prgrom[0x3cc] = 0xde;
	prgrom[0x3cd] = 0x5d;

	prgrom[0x3b7]=0xfb;
	prgrom[0x3b8]=0xed;
	prgrom[0x3b9]=0x4d;

	timer_set(1.0/60.0, 0, Hexion_Vblank);
}

static void Hexion_SendCmd (int cmda, int cmdb) {
	memset(hex_mem,0,0x2000);
	prgrom[0x0d] = cmda & 0xff;
	
	hex_mem[0x1fcf] = cmda & 0xff;
	cpu_set_reset_line(0,0xFF);
}

// teh drivah
M1_BOARD_START ( hexion )
	MDRV_NAME("Hexion")
	MDRV_HWDESC ("Z80, K051649, OKIM6295")
	MDRV_SEND( Hexion_SendCmd )
	MDRV_INIT( Hexion_Init )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY ( hexion_readmem, hexion_writemem )
	
	MDRV_SOUND_ADD(K051649, &k051649_interface)
	MDRV_SOUND_ADD (OKIM6295, &okim6295_interface)
M1_BOARD_END
