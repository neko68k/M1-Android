/*
	Technosoft Hyper Duel: 68000, YM-2151 and OKIM6295

	The sound/auxillary 68000 has no ROM access.

	The sound program is uploaded to shared RAM (starting at
	0xc00000) by the host and the slave released from reset.

	During the sound test, song data is uploaded to shared RAM
	by the host. It writes 2 or 3 to shared RAM location
	0xc000408 to start/stop the song respectively.
*/

#include "m1snd.h"

#define SOUND_CLK	(4000000)

static void HyperDuel_Init(long srate);
static void HyperDuel_SendCmd(int cmda, int cmdb);

UINT8 ym2151_last_adr;


static void YM2151_IRQ(int irq)
{
	if (irq)
		m68k_set_irq(M68K_IRQ_1);
	else
		m68k_set_irq(M68K_IRQ_NONE);
}

static struct YM2151interface ym2151_interface =
{
	1,
	SOUND_CLK,
	{ YM3012_VOL(80, MIXER_PAN_LEFT, 80, MIXER_PAN_RIGHT)},
	{ YM2151_IRQ }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ SOUND_CLK/16/16 },
	{ RGN_SAMP1 },
	{ 57 }
};


static unsigned int read_memory_8(unsigned int address)
{
	/* Mirror of C00000 - C03FFFF */
	if (address < 0x004000)
		return workram[address];

	/* Read only? mirror of FE4000 - FE7FFFF */
	else if ((address >= 0x4000) && (address < 0x8000))
		return workram[0x10000 + (address & 0x3fff)];

	else if (address == 0x400003)
		return YM2151ReadStatus(0);

	else if (address == 0x40005)
		return OKIM6295_status_0_r(0);

	else if ((address >= 0xc00000) && (address < 0xc08000))
		return workram[address & 0x7fff];

	else if ((address >= 0xfe0000) && (address < 0xffffff))
		return workram[0x10000 + (address & 0x1ffff)];

	printf("R8 @ %x\n", address);
	return 0xff;
}

static unsigned int read_memory_16(unsigned int address)
{
	if (address < 0x004000)
		return mem_readword_swap((unsigned short *)(workram + address));

	else if ((address >= 0x4000) && (address < 0x8000))
	{
		address &= 0x3ffff;
		return mem_readword_swap((unsigned short *)(workram + 0x10000 + address));
	}
	else if ((address >= 0xc00000) && (address < 0xc08000))
	{
		address &= 0x7fff;
		return mem_readword_swap((unsigned short *)(workram + address));
	}
	else if ((address >= 0xfe0000) && (address < 0xffffff))
	{
		address &= 0x1ffff;
		return mem_readword_swap((unsigned short *)(workram + 0x10000 + address));
	}

	printf("R16 @ %x\n", address);
	return 0xffff;
}

static unsigned int read_memory_32(unsigned int address)
{
	if (address < 0x004000)
		return mem_readlong_swap((unsigned int *)(workram + address));

	else if ((address >= 0x4000) && (address < 0x8000))
	{
		address &= 0x3ffff;
		return mem_readlong_swap((unsigned int *)(workram + 0x10000 + address));
	}
	else if ((address >= 0xc00000) && (address < 0xc08000))
	{
		address &= 0x7fff;
		return mem_readlong_swap((unsigned int *)(workram + address));
	}
	else if ((address >= 0xfe0000) && (address < 0xffffff))
	{
		address &= 0x1ffff;
		return mem_readlong_swap((unsigned int *)(workram + 0x10000 + address));
	}

	printf("R32 @ %x\n", address);
	return 0xffffffff;
}

static void write_memory_8(unsigned int address, unsigned int data)
{
	if (address < 0x004000)
		workram[address] = data;

	else if ((address >= 0x4000) && (address < 0x8000))
		workram[0x10000 + (address & 0x3fff)] = data;

	else if (address == 0x400001)
		ym2151_last_adr = data; 

	else if (address == 0x400003)
		YM2151WriteReg(0, ym2151_last_adr, data);

	else if (address == 0x400005)
		OKIM6295_data_0_w(0, data);
	
	else if ((address >= 0xc00000) && (address < 0xc08000))
		workram[address & 0x7fff] = data;

	else if ((address >= 0xfe0000) && (address < 0xffffff))
		workram[0x10000 + (address & 0x1ffff)] = data;

	else
		printf("W8: %x @ %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void write_memory_16(unsigned int address, unsigned int data)
{
	if (address < 0x004000)
		mem_writeword_swap((unsigned short *)(workram + address), data);

	else if ((address >= 0x4000) && (address < 0x8000))
	{
		address &= 0x3ffff;
		mem_writeword_swap((unsigned short *)(workram + 0x10000 + address), data);
	}
	else if ((address >= 0xc00000) && (address < 0xc08000))
	{
		address &= 0x7fff;
		mem_writeword_swap((unsigned short *)(workram + address), data);
	}
	else if(address == 0x800000)
	{
		return;
	}
	else if ((address >= 0xfe0000) && (address < 0xffffff))
	{
		address &= 0x1ffff;
		mem_writeword_swap((unsigned short *)(workram + 0x10000 + address), data);
	}
	else
		printf("W16: %x @ %x\n", data, address);
}

static void write_memory_32(unsigned int address, unsigned int data)
{
	if (address < 0x004000)
	{
		mem_writelong_swap((unsigned int *)(workram + address), data);
	}
	else if ((address >= 0x4000) && (address < 0x8000))
	{
		address &= 0x3ffff;
		mem_writelong_swap((unsigned int *)(workram + 0x10000 + address), data);
	}
	else if ((address >= 0xc00000) && (address < 0xc08000))
	{
		address &= 0x7fff;
		mem_writelong_swap((unsigned int *)(workram + address), data);
	}
	else if ((address >= 0xfe0000) && (address < 0xffffff))
	{
		address &= 0x1ffff;
		mem_writelong_swap((unsigned int *)(workram + 0x10000 + address), data);
	}
	else
		printf("W32: %x @ %x\n", data, address);
}


static M168KT readwritemem =
{
	read_memory_8,
	read_memory_16,
	read_memory_32,
	write_memory_8,
	write_memory_16,
	write_memory_32,
};


M1_BOARD_START( hyperduel )
	MDRV_NAME("Hyper Duel")
	MDRV_HWDESC("68000, YM2151, MSM6295")
	MDRV_INIT( HyperDuel_Init )
	MDRV_SEND( HyperDuel_SendCmd )

	MDRV_CPU_ADD(MC68000, 10000000)
	MDRV_CPUMEMHAND(&readwritemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END


static void HyperDuel_Init(long srate)
{
	UINT8	*rom;
	UINT8	b1;
	UINT16	w1, w2, w3, w4;

	/* The program lives at ROM bank 0x3E */
	rom = prgrom + 0x10000 * 0x3e;

	/* Host 68000 program lives at 0x009c40, for future ref... */
	w1 = mem_readword_swap((unsigned short*)&rom[0x4]);
	w2 = mem_readword_swap((unsigned short*)&rom[0x6]);
	w3 = mem_readword_swap((unsigned short*)&rom[0x8]);
	w4 = mem_readword_swap((unsigned short*)&rom[0xa]);	
	b1 = rom[0x7 + w2];

	memcpy(workram, rom + w1, w3 - 1);

	mem_writelong_swap((unsigned int*)&workram[0x41a], 0xc00000 + w3);
	mem_writelong_swap((unsigned int*)&workram[0x41e], 0xc00000 + w2 - w1);
	mem_writeword_swap((unsigned short*)&workram[0x412], w4);
	mem_writeword_swap((unsigned short*)&workram[0x414], b1);
	mem_writeword_swap((unsigned short*)&workram[0x436], 0x3e);
}


static void HyperDuel_SendCmd(int cmda, int cmdb)
{
	/* Music */
	if (cmda < 21)
	{		
		UINT32	offset;
		UINT16	size;
		UINT8	*src;
		UINT8	*dst;
		UINT8	*rom = prgrom + 0x10000 * 0x3e;

		/* Copy music data from ROM to RAM */
		cmda = (cmda << 2) + 0xc;
		offset = mem_readlong_swap((unsigned int*)&rom[cmda]);
		size = mem_readword_swap((unsigned short*)&rom[0xa + offset]);
		
		src = &rom[offset];
		dst = &workram[mem_readlong_swap((unsigned int*)&workram[0x41a]) & 0x7fff];
		
		memcpy(dst, src, size);

		/* Play command */
		workram[0x408] = 0x2;
	}
	/* Sound effects */
	else if( cmda < 106 )
	{
		UINT16 ptr = mem_readword_swap((unsigned short*)&workram[0x43c]);

		/* Write command to queue */
		workram[0x2b86 + ptr] = cmda - 21;
		workram[0x2b87 + ptr] = 0;

		/* Update queue pointer */
		ptr = (ptr + 2) & 0x3f;
		mem_writeword_swap((unsigned short*)&workram[0x43c], ptr);
	}
	/* Stop music/sound command */
	else if (cmda == 255)
	{		
		UINT16 ptr = mem_readword_swap((unsigned short*)&workram[0x43c]);

		/* Stop music */
		workram[0x408] = 0x3;
		
		/* Stop sound and update queue pointer */
		workram[0x2b86 + ptr] = 0xff;
		workram[0x2b87 + ptr] = 0xff;
		mem_writeword_swap((unsigned short*)&workram[0x43c], (ptr + 2) & 0x3f);
	}
}
