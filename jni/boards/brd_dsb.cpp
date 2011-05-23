/* Sega MPEG "Digital Sound Board" DSB-2 */

/*
	Work RAM of interest, at least in Sega Rally 2..

	f01008.b: command byte to process
	f01009.b: if bit 7 is set, there's a command waiting to be parsed at f01008
	f0100a.l: function pointer for incoming command byte state machine
	f0100e.w: timer, incremented once per IRQ2
	f01010.b: shadow of LED/board control register at d00001
	f01011.b: number of pending command bytes

	NOTE: interpretation of the Ax / Bx commands is very imperfect,
	but it works for the currently supported games.
*/

#include "m1snd.h"
#include "mpeg.h"

static int cmd_latch;

enum 
{
	ST_IDLE = 0,
	ST_GOT14,	// start/loop addr
	ST_14_0,
	ST_14_1,
	ST_GOT24,	// end addr
	ST_24_0,
	ST_24_1,
	ST_GOT74,
	ST_GOTA0,
	ST_GOTA1,
	ST_GOTA4,
	ST_GOTA5,
	ST_GOTB0,
	ST_GOTB1,
	ST_GOTB4,
	ST_GOTB5,
};

static int mpeg_state = ST_IDLE;
static int mp_start, mp_end, playing;

static void write_mpeg_fifo(int byte)
{
	char *ROM = (char *)memory_region(RGN_SAMP1);

//	printf("fifo: %x (state %d)\n", byte, mpeg_state);
	switch (mpeg_state)
	{
		case ST_IDLE:
			if (byte == 0x14) mpeg_state = ST_GOT14;
			else if (byte == 0x15) mpeg_state = ST_GOT14;
			else if (byte == 0x24) mpeg_state = ST_GOT24;
			else if (byte == 0x25) mpeg_state = ST_GOT24;

			else if (byte == 0x74 || byte == 0x75)	// "start play"
			{
				MPEG_Play_Memory(ROM + mp_start, mp_end-mp_start);
				mpeg_state = ST_IDLE;
				playing = 1;
			}

			else if (byte == 0x84 || byte == 0x85)
			{
				MPEG_Stop_Playing();
				playing = 0;
			}

			else if (byte == 0xa0) mpeg_state = ST_GOTA0;
			else if (byte == 0xa1) mpeg_state = ST_GOTA1;
			else if (byte == 0xa4) mpeg_state = ST_GOTA4;
			else if (byte == 0xa5) mpeg_state = ST_GOTA5;

			else if (byte == 0xb0) mpeg_state = ST_GOTB0;
			else if (byte == 0xb1) mpeg_state = ST_GOTB1;
			else if (byte == 0xb4) mpeg_state = ST_GOTB4;
			else if (byte == 0xb5) mpeg_state = ST_GOTB5;

			break;

		case ST_GOT14:
			mp_start &= ~0xff0000;
			mp_start |= (byte<<16);
			mpeg_state++;
			break;
		case ST_14_0:
			mp_start &= ~0xff00;
			mp_start |= (byte<<8);
			mpeg_state++;
			break;
		case ST_14_1:
			mp_start &= ~0xff;
			mp_start |= (byte);
			mpeg_state = ST_IDLE;

			if (playing)
			{
//				printf("Setting loop point to %x\n", mp_start);
				MPEG_Set_Loop(ROM + mp_start, mp_end-mp_start);
			}

//			printf("mp_start=%x\n", mp_start);
			break;
		case ST_GOT24:
			mp_end &= ~0xff0000;
			mp_end |= (byte<<16);
			mpeg_state++;
			break;
		case ST_24_0:
			mp_end &= ~0xff00;
			mp_end |= (byte<<8);
			mpeg_state++;
			break;
		case ST_24_1:
			mp_end &= ~0xff;
			mp_end |= (byte);
//			printf("mp_end=%x\n", mp_end);

			// default to full stereo
			mixer_set_stereo_volume(0, 255, 255);
			mixer_set_stereo_pan(0, MIXER_PAN_RIGHT, MIXER_PAN_LEFT);
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTA0:
			// ch 0 mono
			mixer_set_stereo_volume(0, 0, 255);
//			printf("ch 0 mono\n");
			mixer_set_stereo_pan(0, MIXER_PAN_CENTER, MIXER_PAN_CENTER);
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTA1:
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTA4:
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTA5:
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTB0:
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTB1:
			// ch 1 mono
//			printf("ch 1 mono\n");
			mixer_set_stereo_volume(0, 255, 0);
			mixer_set_stereo_pan(0, MIXER_PAN_CENTER, MIXER_PAN_CENTER);
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTB4:
			mpeg_state = ST_IDLE;
			break;
		case ST_GOTB5:
			mpeg_state = ST_IDLE;
			break;
		default:
			break;
	}
}

static unsigned int read_memory_8(unsigned int address)
{
	if (address < (128*1024))
		return prgrom[address];

	if (address == 0xc00001)	
	{
		return cmd_latch;
	}
	if (address == 0xc00003)	// bit 0 = command valid
	{
		return 1;
	}

	if (address == 0xe80001)
	{
		return 0x01;	// MPEG busy status: bit 1 = busy
	}			// polled by irq2, stored | 0x10 at f01010

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0x1ffff;
		return workram[address];
	}

//	printf("R8 @ %x\n", address);
	return 0;
}

static unsigned int read_memory_16(unsigned int address)
{
	if (address < (128*1024))
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}
	if ((address >= 0xf00000) && (address < 0xf20000))
	{
		address &= 0x1ffff;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

//	printf("R16 @ %x\n", address);
	return 0;
}

static unsigned int read_memory_32(unsigned int address)
{
	if (address < (128*1024))
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}
	if ((address >= 0xf00000) && (address < 0xf20000))
	{
		address &= 0x1ffff;
		return mem_readlong_swap((unsigned int *)(workram+address)); 
	}
//	printf("R32 @ %x\n", address);
	return 0;
}

static void write_memory_8(unsigned int address, unsigned int data)
{
	if ((address >= 0xf00000) && (address < 0xf20000))
	{
		address &= 0x1ffff;
		workram[address] = data;
		return;
	}
	
	if (address == 0xd00001) return;

	if (address == 0xe00003) 
	{
		write_mpeg_fifo(data);
		return;
	}

//	printf("W8: %x @ %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void write_memory_16(unsigned int address, unsigned int data)
{
	if ((address >= 0xf00000) && (address < 0xf20000))
	{
		address &= 0x1ffff;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}
//	printf("W16: %x @ %x\n", data, address);
}

static void write_memory_32(unsigned int address, unsigned int data)
{
	if ((address >= 0xf00000) && (address < 0xf20000))
	{
		address &= 0x1ffff;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}
//	printf("W32: %x @ %x\n", data, address);
}

static void DSB_Init(long srate)
{
	mpeg_state = ST_IDLE;
	playing = 0;
}

// hook the per-frame update
static void DSB_Update(long dsppos, long dspframes)
{
	m68k_set_irq(M68K_IRQ_2);
	StreamSys_Update(dsppos, dspframes);
}

static void DSB_SendCmd(int cmda, int cmdb)
{
	cmd_latch = 0xae;
	m68k_set_irq(M68K_IRQ_1);
	m68k_execute(5000);

       	cmd_latch = 0x10;
	m68k_set_irq(M68K_IRQ_1);
	m68k_execute(5000);

	cmd_latch = cmda;
	m68k_set_irq(M68K_IRQ_1);
	m68k_execute(5000);
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

M1_BOARD_START( dsb )
	MDRV_NAME("Sega Digital Sound Board (type 2)")
	MDRV_HWDESC("68000, Sega 315-5762")
	MDRV_DELAYS(500, 100)
	MDRV_INIT( DSB_Init )
	MDRV_UPDATE( DSB_Update )
	MDRV_SEND( DSB_SendCmd )

	MDRV_CPU_ADD(MC68000, 8000000)
	MDRV_CPUMEMHAND(&readwritemem)

	MDRV_SOUND_ADD(MPEG, NULL)
M1_BOARD_END

