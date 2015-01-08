// Atari Hard Drivin' "Driver Sound" board (MC68000 + TMS32020 DSP)

#include "m1snd.h"

static void DRS_Init(long srate);
static void DRS_SendCmd(int cmda, int cmdb);

static unsigned int drs_read_memory_8(unsigned int address);
static unsigned int drs_read_memory_16(unsigned int address);
static unsigned int drs_read_memory_32(unsigned int address);
static void drs_write_memory_8(unsigned int address, unsigned int data);
static void drs_write_memory_16(unsigned int address, unsigned int data);
static void drs_write_memory_32(unsigned int address, unsigned int data);

static int cmd_latch;
static UINT8 cramen;
static UINT8 irq68k;
static UINT8 mainflag;
static UINT8 *rombase;
static UINT32 romsize;
static UINT16 *comram;
static UINT16 hdsnddsp_ram[0x2000];

#if 0
static MEMORY_READ16_START( driversnd_readmem_dsp )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x0ff), MRA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0xfff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driversnd_writemem_dsp )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x0ff), MWA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0xfff), MWA16_RAM, &hdsnddsp_ram },
MEMORY_END

static PORT_READ16_START( driversnd_readport_dsp )
	{ TMS32010_PORT_RANGE(0, 0), hdsnddsp_rom_r },
	{ TMS32010_PORT_RANGE(1, 1), hdsnddsp_comram_r },
	{ TMS32010_PORT_RANGE(2, 2), hdsnddsp_compare_r },
	{ TMS32010_BIO, TMS32010_BIO, hdsnddsp_get_bio },
PORT_END


static PORT_WRITE16_START( driversnd_writeport_dsp )
	{ TMS32010_PORT_RANGE(0, 0), hdsnddsp_dac_w },
	{ TMS32010_PORT_RANGE(1, 2), MWA16_NOP },
	{ TMS32010_PORT_RANGE(3, 3), hdsnddsp_comport_w },
	{ TMS32010_PORT_RANGE(4, 4), hdsnddsp_mute_w },
	{ TMS32010_PORT_RANGE(5, 5), hdsnddsp_gen68kirq_w },
	{ TMS32010_PORT_RANGE(6, 7), hdsnddsp_soundaddr_w },
PORT_END
#endif

static M168KT drs_readwritemem =
{
	drs_read_memory_8,
	drs_read_memory_16,
	drs_read_memory_32,
	drs_write_memory_8,
	drs_write_memory_16,
	drs_write_memory_32,
};

static struct DACinterface dac_interface =
{
	1,
	{ MIXER(100, MIXER_PAN_CENTER) }
};

M1_BOARD_START( harddriv )
	MDRV_NAME("Atari Driver Sound")
	MDRV_HWDESC("68000, TMS32010")
	MDRV_INIT( DRS_Init )
	MDRV_SEND( DRS_SendCmd )

	MDRV_CPU_ADD(MC68000, 8000000)
	MDRV_CPUMEMHAND(&drs_readwritemem)

	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static unsigned int drs_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x20000)
	{
		return prgrom[address];
	}

	if (address >= 0xff3000 && address <= 0xff3fff)
	{
		if (address & 1)
			return 0;
		else
			return (mainflag << 7) | 0x20;
	}

	if ((address >= 0xffc000) && (address <= 0xffffff))
	{
		address -= 0xffc000;
		return workram[address];
	}

	printf("Unknown read 8 at %x\n", address);
	return 0;
}

static unsigned int drs_read_memory_16(unsigned int address)
{
	int offset;

	address &= 0xffffff;

	offset = address & 0x0fff;
	offset <<= 1;

	if (address < 0x20000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if (address >= 0xff0000 && address <= 0xff0fff)
	{
		mainflag = 0;
		printf("reading latch\n");
		return cmd_latch;
	}

	if (address >= 0xff1000 && address <= 0xff1fff)
	{
		return 0;
	}

	if (address >= 0xff2000 && address <= 0xff2fff)
	{
		return 0;
	}

	if (address >= 0xff3000 && address <= 0xff3fff)
	{
		return (mainflag << 15) | 0x2000;
	}
	
	if (address >= 0xff4000 && address <= 0xff5fff)
	{
		address -= 0xff4000;
		return mem_readword_swap((unsigned short *)(hdsnddsp_ram+address));
	}

	if (address >= 0xff6000 && address <= 0xff7fff)
	{
		printf("Read 32010 port %x\n", offset & 7);
//		return TMS32010_In(offset & 7);
		return 0;
	}

	if (address >= 0xff8000 && address <= 0xffbfff)
	{
		if (cramen)
		{
			address -= 0xff8000;
			return mem_readword_swap((unsigned short *)(comram+address));
		}

		return 0xffff;
	}

	if ((address >= 0xffc000) && (address <= 0xffffff))
	{
		address -= 0xffc000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	printf("Unknown read 16 at %x\n", address);
	return 0;
}

static unsigned int drs_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x20000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if ((address >= 0xffc000) && (address <= 0xffffff))
	{
		address -= 0xffc000;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

	printf("Unknown read 32 at %x\n", address);
	return 0;
}

static void drs_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if ((address >= 0xffc000) && (address <= 0xffffff))
	{
		address -= 0xffc000;
		workram[address] = data;
		return;
	}

	printf("Unknown write 8 %x to %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void drs_write_memory_16(unsigned int address, unsigned int data)
{
	int offset;

	address &= 0xffffff;

	offset = address & 0x0fff;
	offset <<= 1;

	if (address >= 0xff0000 && address <= 0xff0fff)
	{
		return;	// write to main 68k
	}

	if (address >= 0xff1000 && address <= 0xff1fff)
	{
		data = (offset >> 3) & 1;

		offset &= 7;
		switch (offset)
		{
			case 3:
				cramen = data;
				break;

			case 4:	// reset the 32010
				printf("Resetting 32010: %d\n", data);
				break;

			case 7:	// LEDs
				break;
		}
		return;
	}

	if (address >= 0xff2000 && address <= 0xff2fff)
	{
		return;
	}

	if (address >= 0xff3000 && address <= 0xff3fff)
	{
		irq68k = 0;
		return;
	}
	
	if (address >= 0xff4000 && address <= 0xff5fff)
	{
		address -= 0xff4000;
		mem_writeword_swap((unsigned short *)(hdsnddsp_ram+address), data);
		return;
	}

	if (address >= 0xff6000 && address <= 0xff7fff)
	{
//		TMS32010_Out(offset & 7, data);
		printf("write %x to 32010 port %x\n", data, offset & 7);
		return;
	}

	if (address >= 0xff8000 && address <= 0xffbfff)
	{
		if (cramen)
		{
			address -= 0xff8000;
			mem_writeword_swap((unsigned short *)(comram+address), data);
		}
		return;
	}

	if ((address >= 0xffc000) && (address <= 0xffffff))
	{
		address -= 0xffc000;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}

	printf("Unknown write 16 %x to %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC)); 
}

static void drs_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0xff8000 && address <= 0xffbfff)
	{
		if (cramen)
		{
			address -= 0xff8000;
			mem_writelong_swap((unsigned int *)(comram+address), data);
		}
		return;
	}

	if ((address >= 0xffc000) && (address <= 0xffffff))
	{
		address -= 0xffc000;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}

	printf("Unknown write 32 %x to %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC)); 
}

static void DRS_Init(long srate)
{
	irq68k = 0;
	mainflag = 0;
	rombase = (UINT8 *)memory_region(RGN_SAMP1);
	romsize = memory_region_length(RGN_SAMP1);
	comram = (UINT16 *)(memory_region(RGN_CPU2) + 0x1000);
}

static void DRS_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	mainflag = 1;
	m68k_set_irq(M68K_IRQ_1);
}
