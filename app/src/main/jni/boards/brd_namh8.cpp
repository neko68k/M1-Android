// namco H8/3002 sound boards (System 12, ND-1)

#include "m1snd.h"

static void BB_SendCmd(int cmda, int cmdb);

static unsigned int nh8_read_memory_8(unsigned int address);
static unsigned int nh8_read_memory_16(unsigned int address);
static void nh8_write_memory_8(unsigned int address, unsigned int data);
static void nh8_write_memory_16(unsigned int address, unsigned int data);

static unsigned int nd1h8_read_memory_8(unsigned int address);
static unsigned int nd1h8_read_memory_16(unsigned int address);
static void nd1h8_write_memory_8(unsigned int address, unsigned int data);
static void nd1h8_write_memory_16(unsigned int address, unsigned int data);

extern "C" {
UINT8 h8_register_read8(UINT32 address);
void h8_register_write8(UINT32 address, UINT8 val);
};

#define H8_REG_START	(0x00ffff10)

static int cmd_latch;

static unsigned char unkram[256];

static M1H83K2T nh8_readwritemem =
{
	nh8_read_memory_8,
	nh8_read_memory_16,
	nh8_write_memory_8,
	nh8_write_memory_16,
};

static M1H83K2T nd1h8_readwritemem =
{
	nd1h8_read_memory_8,
	nd1h8_read_memory_16,
	nd1h8_write_memory_8,
	nd1h8_write_memory_16,
};

static unsigned int nh8_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 512*1024)
	{
		return prgrom[address];
	}

	if (address >= 0x80000 && address < 0x90000)
	{
//		if (address >= 0x83000 && address <= 0x83010)
//			printf("[RB %06x]\n", address);
		return workram[address-0x80000];
	}

	// unknown
	if (address >= 0x300000 && address <= 0x3000ff)
	{
		return unkram[address&0xff];
	}

	if (address >= 0xfffd10 && address < 0xffff10)
	{
		return workram[address-0xfffd10+0x80000];
	}

	if (address >= H8_REG_START && address <= 0xffffffff)
	{
		return h8_register_read8(address);
	}

//	printf("Unknown read 8 at %x\n", address);

	return 0;
}

static unsigned int nh8_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < (512*1024))
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if (address >= 0x80000 && address < 0x90000)
	{
//		if (address >= 0x83000 && address <= 0x83010)
//			printf("[RW %06x]\n", address);
		address -= 0x80000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address >= 0x280000 && address <= 0x287fff)
	{
		return c352_0_r((address&0xffff)>>1, 0);
	}

	if (address >= 0xfffd10 && address < 0xffff10)
	{
		address = address - 0xfffd10 + 0x80000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address >= H8_REG_START && address <= 0xffffffff)
	{
		return h8_register_read8(address)<<8 | h8_register_read8(address+1);
	}

//	printf("Unknown read 16 at %x\n", address);
	return 0;
}

static void nh8_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x80000 && address < 0x90000)
	{
//		if (address >= 0x83000 && address <= 0x83010)
//			printf("[%06x]: %02x\n", address, data);
		workram[address-0x80000] = data;
		return;
	}

	if (address >= 0x300000 && address <= 0x3000ff)
	{
		unkram[address&0xff] = data;
		return;
	}

	// chip ram
	if (address >= 0xfffd10 && address < 0xffff10)
	{
		workram[address-0xfffd10+0x80000] = data;
		return;
	}

	if (address >= H8_REG_START && address <= 0xffffffff)
	{
		h8_register_write8(address, data);
		return;
	}

//	printf("Unknown write 8 %x to %x\n", data, address);
}

static void nh8_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x80000 && address < 0x90000)
	{
//		if (address >= 0x83000 && address <= 0x83010)
//			printf("[%06x]: %04x\n", address, data);
		address -= 0x80000;
		return mem_writeword_swap((unsigned short *)(workram+address), data);
	}

	if (address >= 0x280000 && address <= 0x287fff)
	{
		c352_0_w((address&0xffff)>>1, data, 0);
		return;
	}

	if (address >= 0xfffd10 && address < 0xffff10)
	{
		address = address - 0xfffd10 + 0x80000;
		return mem_writeword_swap((unsigned short *)(workram+address), data);
	}

	if (address >= H8_REG_START && address <= 0xffffffff)
	{
		h8_register_write8(address, data>>8);
		h8_register_write8(address+1, data&0xff);
		return;
	}

//	printf("Unknown write 16 %x to %x\n", data, address);
}

// ND-1 SYSTEM

static unsigned int nd1h8_read_memory_8(unsigned int address)
{
	if (address < 512*1024)
	{
		return prgrom[address];
	}

	if (address >= 0x200000 && address < 0x300000)
	{
		return workram[address-0x200000];
	}

#if 0
	// unknown
	if (address >= 0x300000 && address <= 0x3000ff)
	{
		return unkram[address&0xff];
	}
#endif

	if (address >= 0xfffd10 && address < 0xffff10)
	{
		return workram[address-0xfffd10+0x80000];
	}

	if (address >= H8_REG_START)
	{
		return h8_register_read8(address);
	}

//	printf("Unknown read 8 at %x\n", address);
	return 0;
}

static unsigned int nd1h8_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < (512*1024))
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if (address >= 0x200000 && address < 0x300000)
	{
		address -= 0x200000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address >= 0xa00000 && address <= 0xa07fff)
	{
		return c352_0_r((address&0xffff)>>1, 0);
	}

	if (address >= 0xfffd10 && address < 0xffff10)
	{
		address = address - 0xfffd10 + 0x80000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address >= H8_REG_START)
	{
		return h8_register_read8(address)<<8 | h8_register_read8(address+1);
	}

//	printf("Unknown read 16 at %x\n", address);
	return 0;
}

static void nd1h8_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x200000 && address < 0x300000)
	{
		workram[address-0x200000] = data;
		return;
	}
#if 0
	if (address >= 0x300000 && address <= 0x3000ff)
	{
		unkram[address&0xff] = data;
		return;
	}
#endif

	// chip ram
	if (address >= 0xfffd10 && address < 0xffff10)
	{
		workram[address-0xfffd10+0x80000] = data;
		return;
	}

	if (address >= H8_REG_START)
	{
		h8_register_write8(address, data);
		return;
	}

//	printf("Unknown write 8 %x to %x\n", data, address);
}

static void nd1h8_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address < 0x80000) return;

	if (address >= 0x200000 && address < 0x300000)
	{
		address -= 0x200000;
		return mem_writeword_swap((unsigned short *)(workram+address), data);
	}

	if (address >= 0xa00000 && address <= 0xa07fff)
	{
		c352_0_w((address&0xffff)>>1, data, 0);
		return;
	}

	if (address >= 0xfffd10 && address < 0xffff10)
	{
		address = address - 0xfffd10 + 0x80000;
		return mem_writeword_swap((unsigned short *)(workram+address), data);
	}

	if (address >= H8_REG_START)
	{
		h8_register_write8(address, data>>8);
		h8_register_write8(address+1, data&0xff);
		return;
	}

//	printf("Unknown write 16 %x to %x\n", data, address);
}

#if 1
static void gx_timer(int refcon)
{
	timer_set(1.0/60.0, 0, gx_timer);

	h8_3002_InterruptRequest(13);	// IRQ 1 for S12
//	h8_3002_InterruptRequest(17);	// IRQ 5 for ND-1
}
#endif

static void BB_Init(long srate)
{
#if 0
	FILE *f;

	prgrom = memory_region(REGION_CPU1);

	f = fopen("h8prg.bin", "wb");
	fwrite(prgrom, 512*1024, 1, f);
	fclose(f);
#endif

	// patch bad? code at 10b0 in ncv1
//	if (Machine->refcon == 1)
//	{
//		// replace "bt self" with "nop"
//		prgrom[0x10b0] = 0;
//		prgrom[0x10b1] = 0;
//	}
	timer_set(1.0/60.0, 0, gx_timer);
}

static struct C352interface c352s12_interface =
{
	YM3012_VOL(150, MIXER_PAN_LEFT, 150, MIXER_PAN_RIGHT),
	YM3012_VOL(150, MIXER_PAN_LEFT, 150, MIXER_PAN_RIGHT),
	REGION_SOUND1,
	16934400 //14745600	// system 12
};

static struct C352interface c352_interface =
{
	YM3012_VOL(150, MIXER_PAN_LEFT, 150, MIXER_PAN_RIGHT),
	YM3012_VOL(150, MIXER_PAN_LEFT, 150, MIXER_PAN_RIGHT),
	REGION_SOUND1,
	16384000	// system 12, 16384000 = nd-1 & system 22
};

M1_BOARD_START( system12 )
	MDRV_NAME("Namco System 12")
	MDRV_HWDESC("H8/3002, C352")
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(H8_3002, 16684000/2)
	MDRV_CPUMEMHAND(&nh8_readwritemem)

	MDRV_SOUND_ADD(C352, &c352s12_interface)
M1_BOARD_END

M1_BOARD_START( namcond1 )
	MDRV_NAME("Namco ND-1")
	MDRV_HWDESC("H8/3002, C352")
	MDRV_INIT( BB_Init )
	MDRV_SEND( BB_SendCmd )

	MDRV_CPU_ADD(H8_3002, 16384000/2)
	MDRV_CPUMEMHAND(&nd1h8_readwritemem)

	MDRV_SOUND_ADD(C352, &c352_interface)
M1_BOARD_END

static void BB_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;	

	workram[0x101] = cmda & 0xff;
	workram[0x100] = 0x40 | (cmda&0xf00)>>8;

	nh8_write_memory_16(0x3002, 0x3163);
}

