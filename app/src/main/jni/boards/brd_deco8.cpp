/* DECO 8-bit systems w/6502 for sound */

// IRQ from 3812, NMI on command

#include "m1snd.h"

#define M6502_CLOCK (1500000)
#define YM_CLOCK    (1500000)
#define YM3812_CLOCK (3000000)

static void DECO8_Init(long srate);
static void DECO8_SendCmd(int cmda, int cmdb);
static void YM3812IRQ(int irq);

static unsigned int deco8_readmem(unsigned int address);
static unsigned int deco8_readop(unsigned int address);
static unsigned int deco8_nocrypt_readop(unsigned int address);
static void deco8_writemem(unsigned int address, unsigned int data);

static int cmd_latch;

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* Should be accurate for all games, derived from 12MHz crystal */
	{ YM2203_VOL(20,23) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,		/* 1 chip */
	3000000,	/* 3 MHz */
	{ 70 },
	{ YM3812IRQ },
};

static M16502T btrw = 
{
	deco8_readmem,
	deco8_readop,
	deco8_writemem,
};

static M16502T nocryptrw = 
{
	deco8_readmem,
	deco8_nocrypt_readop,
	deco8_writemem,
};

static unsigned int deco8_readmem(unsigned int address)
{
	if (address < 0x600)
	{	
		return workram[address];
	}
	
	if (address == 0x6000)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x8000)
	{
		return prgrom[address-0x8000];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static unsigned int deco8_readop(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom[(address-0x8000)+0x10000];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static unsigned int deco8_nocrypt_readop(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom[(address-0x8000)];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static void deco8_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x600)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x2000 && address <= 0x2001)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x4000 && address <= 0x4001)
	{
		YM3812Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x8000)
	{
		return;
	}

//	printf("unk write %x at %x\n", data, address);
}

/* YM3812 IRQ handler */
static void YM3812IRQ(int irq)
{
//	printf("OPL IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);
}

static void decrypt_deco8(void)
{
	int A,diff;
	unsigned char *rom;

	/* bits 5 and 6 of the opcodes are swapped */
	rom = rom_getregion(RGN_CPU1);
	diff = rom_getregionsize(RGN_CPU1) / 2;

	for (A = 0;A < 0x10000;A++)
		rom[A + diff] = (rom[A] & 0x9f) | ((rom[A] & 0x20) << 1) | ((rom[A] & 0x40) >> 1);
}

static void DECO8_Init(long srate)
{
	if (Machine->refcon == 1)
	{
		decrypt_deco8();
		m1snd_add6502(M6502_CLOCK, &btrw);
	}
	else
	{
		m1snd_add6502(M6502_CLOCK, &nocryptrw);
	}
}

static void DECO8_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

M1_BOARD_START( deco8 )
	MDRV_NAME("DECO8")
	MDRV_HWDESC("6502, YM2203, YM3812")
	MDRV_DELAYS( 1000, 15 )
	MDRV_INIT( DECO8_Init )
	MDRV_SEND( DECO8_SendCmd )

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
M1_BOARD_END

