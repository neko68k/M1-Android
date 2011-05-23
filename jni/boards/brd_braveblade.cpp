// Brave Blade and other Eighing/Raizing "PS Arcade '95" games
// 68000 + YMF 271-F "OPX"
//
// Memory map:
// 000000-0FFFFF : Program ROM (1 meg)
// 080000-083FFF : Work RAM
// 100000-10000f : YMF 271-F OPX
// 180000-18000c : R3k interface.  Write to 180000 echos to R3k, read from R3k at 180008.
//
// IRQs:
// 1 is an RTE
// 2 and 3 are the same (command input from R3k)
// 4-7 all are an instant crash
//
// In Brave Blade:
// TRAP #1 - wait for OPX to not be busy
// TRAP #2 - write d0/d1/d2 to OPX
// TRAP #3 - not sure yet

#include "m1snd.h"

#define M68K_CLOCK   (8000000)

static void BB_Init(long srate);
static void BB_SendCmd(int cmda, int cmdb);

static unsigned int bb_read_memory_8(unsigned int address);
static unsigned int bb_read_memory_16(unsigned int address);
static unsigned int bb_read_memory_32(unsigned int address);
static void bb_write_memory_8(unsigned int address, unsigned int data);
static void bb_write_memory_16(unsigned int address, unsigned int data);
static void bb_write_memory_32(unsigned int address, unsigned int data);

static int cmd_latch;

static M168KT bb_readwritemem =
{
	bb_read_memory_8,
	bb_read_memory_16,
	bb_read_memory_32,
	bb_write_memory_8,
	bb_write_memory_16,
	bb_write_memory_32,
};


static unsigned int bb_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x80000)
	{
		return prgrom[address];
	}

	if (address >= 0x80000 && address <= 0xfffff)
	{
		return workram[address-0x80000];
	}

//	printf("Unknown read 8 at %x PC=%x\n", address, m68k_get_reg(NULL, M68K_REG_PC));

	return 0;
}

static unsigned int bb_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x80000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if ((address >= 0x80000) && (address <= 0xfffff))
	{
		address -= 0x80000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address == 0x100000)
	{
		return YMF271_0_r(0);
	}

	if (address == 0x180008)
	{
		return cmd_latch;
	}

	return 0;
}

static unsigned int bb_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x80000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if ((address >= 0x80000) && (address <= 0xfffff))
	{
		address -= 0x80000;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

//	printf("Unknown read 32 at %x PC=%x\n", address, m68k_get_reg(NULL, M68K_REG_PC));
	return 0;
}

static void bb_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x80000 && address < 0x8ffff)
	{
		address -= 0x80000;
		workram[address] = data;
		return;
	}

//	printf("Unknown write 8 %x to %x PC=%x\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void bb_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x80000 && address <= 0x8ffff)
	{
		address -= 0x80000;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}

	if (address >= 0x100000 && address <= 0x10001e)
	{
		address &= 0xff;

		YMF271_0_w(address>>1, data);
		return;
	}

//	printf("Unknown write 16 %x to %x PC=%x\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void bb_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x80000 && address <= 0x8ffff)
	{
		address -= 0x80000;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}

//	printf("Unknown write 32 %x to %x PC=%x\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void BB_Init(long srate)
{
}

static void BB_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;	

	m68k_set_irq(M68K_IRQ_2);
}

static struct YMF271interface ymf271_interface =
{
	1,
	NULL,
	NULL,
	{ REGION_SOUND1, },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT),},
};

M1_BOARD_START( braveblade )
	MDRV_NAME("Raizing/Eighting PS Arcade '95")
	MDRV_HWDESC("68000, YMF271-F (OPX)")
	MDRV_DELAYS( 2500, 150 )
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(MC68000, 8000000)
	MDRV_CPUMEMHAND(&bb_readwritemem)

	MDRV_SOUND_ADD(YMF271, &ymf271_interface)
M1_BOARD_END

