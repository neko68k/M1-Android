/* -------------------------------------------------------------
 * Sega Model 2 audio memory map (CPU = MC68EC000 at 11.3 MHz)
 *
 * 000000-07ffff : RAM (512k) (68k work + SCSP samples)
 * 100000-100fff : SCSP
 * 400001        : b0-3 are leds, b4 0=1 wait state, 1= no wait states, b5 = rom type (influences the banking), b6-7 ignored
 * 600000-67ffff : program ROM (up to 512k)
 * 800000-9fffff : fixed sample ROM
 * a00000-dfffff : sample ROM, bank 0 window
 * e00000-ffffff : sample ROM, bank 1 window
 *
 * either you have 16Mb roms, and then rom0 is at 600000, rom 1 at 800000, rom 2 at a00000, rom3 at c00000, rom4 at e00000
 * or you have 1 16Mb and 2 32Mb, and you have rom0 at 600000, rom1 at 800000 and rom3 at c00000 (and no rom2 or rom4)  
 * 1 in the bit for the 32Mbit setup
 * 
 * i960 I/O takes place via the SCSP MIDI In port (960 -> 68k).
 * There is no back-channel as far as I know.
 *
 * IRQ assignments (VF2, other games may vary):
 * 1: timer B
 * 2: timer A
 * 3: SCSP MIDI In (caused by commands from i960)
 * 4-7 are an infinite loop and signal error on the LEDs
 *
 * -------------------------------------------------------------
 * Sega Model 3 audio memory map (CPU = MC68EC000 at 11.3 MHz)
 *
 * 000000-07ffff : RAM (512k) (for SCSP #1 + 68k work)
 * 100000-100fff : SCSP #1
 * 200000-27ffff : RAM (512k) (for SCSP #2)
 * 300000-300fff : SCSP #2
 * 400001	 : Control register & LEDs
 * 600000-67ffff : program ROM (512k)
 * 680000-6fffff : mirror of program ROM
 * 800000        : sample ROM (8 megs in VF3)
 *
 * The IRQs and comm protocol are the same as for Model 2.
 *
 */

#include "m1snd.h"
#include "mpeg.h"

#define M68K_CLOCK (11700000)	// 11.7 MHz

static void M2_Init(long srate);
static void M2_SendCmd(int cmda, int cmdb);

static unsigned int m2_read_memory_8(unsigned int address);
static unsigned int m2_read_memory_16(unsigned int address);
static unsigned int m2_read_memory_32(unsigned int address);
static void m2_write_memory_8(unsigned int address, unsigned int data);
static void m2_write_memory_16(unsigned int address, unsigned int data);
static void m2_write_memory_32(unsigned int address, unsigned int data);

static unsigned char scsp1_ram[512*1024], scsp2_ram[512*1024];
static int bInit;

extern int cmd_where;
extern unsigned char cmd[6];

static int bank0, bank1, bankenable;

static void m2_irq_handler(int irq)
{
	if (irq > 0)
	{
		m68k_set_irq(irq);
	}
}

struct SCSPinterface scsp_interface =
{
	2,
	{ scsp1_ram, scsp2_ram },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT), YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) },
	{ m2_irq_handler, m2_irq_handler },
};

static M168KT m2_readwritemem =
{
	m2_read_memory_8,
	m2_read_memory_16,
	m2_read_memory_32,
	m2_write_memory_8,
	m2_write_memory_16,
	m2_write_memory_32,
};

static unsigned int m2_read_memory_8(unsigned int address)
{
	UINT8 *samples = rom_getregion(RGN_SAMP1);

	address &= 0xffffff;

	if (address < (512*1024))
		return scsp1_ram[address];

	if (address >= 0x100000 && address < 0x100fff)
	{
		int foo = SCSP_0_r((address - 0x100000)/2, 0);

		if (address & 1)
			return foo & 0xff;
		else
			return foo>>8;
	}

	if (address >= 0x200000 && address < 0x280000)
		return scsp2_ram[address-0x200000];

	if (address >= 0x300000 && address < 0x300fff)
	{
		int foo = SCSP_1_r((address - 0x300000)/2, 0);

		if (address & 1)
			return foo & 0xff;
		else
			return foo>>8;
	}

	if ((address >= 0x600000) && (address < 0x700000))
	{
		return prgrom[address&0x7ffff];
	}

	if (address >= 0x800000 && address < 0xa00000)
	{
		address -= 0x800000;
		return samples[address];
	}

	if (address >= 0xa00000 && address < 0xe00000)
	{
		address -= 0xa00000;
		return samples[address+bank0];
	}

	if (address >= 0xe00000)
	{
		address -= 0xe00000;
		return samples[address+bank1];
	}

	return 0;
}

static unsigned int m2_read_memory_16(unsigned int address)
{
	UINT8 *samples = rom_getregion(RGN_SAMP1);

	address &= 0xffffff;

	if (address < (512*1024))
	{
		return mem_readword_swap((unsigned short *)(scsp1_ram+address));
	}

	if (address >= 0x100000 && address < 0x100fff)
		return SCSP_0_r((address-0x100000)/2, 0);

	if (address >= 0x200000 && address < 0x280000)
	{
		return mem_readword_swap((unsigned short *)(scsp2_ram+address-0x200000));
	}

	if (address >= 0x300000 && address < 0x300fff)
		return SCSP_1_r((address-0x300000)/2, 0);

	if ((address >= 0x600000) && (address < 0x700000))
	{
		address &= 0x7ffff;
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if (address >= 0x800000 && address < 0xa00000)
	{
		address -= 0x800000;
		return mem_readword_swap((unsigned short *)(samples+address));
	}

	if (address >= 0xa00000 && address < 0xe00000)
	{
		address -= 0xa00000;
		return mem_readword_swap((unsigned short *)(samples+address+bank0));
	}

	if (address >= 0xe00000)
	{
		address -= 0xe00000;
		return mem_readword_swap((unsigned short *)(samples+address+bank1));
	}

	return 0;
}

static unsigned int m2_read_memory_32(unsigned int address)
{
	UINT8 *samples = rom_getregion(RGN_SAMP1);

	address &= 0xffffff;

	if (!bInit)
	{
		// copy down the vectors and stuff
		memcpy(scsp1_ram, prgrom, 512*1024);
		bInit = 1;
	}

	if (address < 0x80000)
	{
		return mem_readlong_swap((unsigned int *)(scsp1_ram+address));
	}

	if (address >= 0x200000 && address < 0x280000)
	{
		return mem_readlong_swap((unsigned int *)(scsp2_ram+address-0x200000));
	}

	if ((address >= 0x600000) && (address < 0x700000))
	{
		address &= 0x7ffff;
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if (address >= 0x800000 && address <= 0xa00000) 
	{
		address -= 0x800000;
		return mem_readlong_swap((unsigned int *)(samples+address));
	}

	if (address >= 0xa00000 && address <= 0xe00000) 
	{
		address -= 0xa00000;
		return mem_readlong_swap((unsigned int *)(samples+address+bank0));
	}

	if (address >= 0xe00000)
	{
		address -= 0xe00000;
		return mem_readlong_swap((unsigned int *)(samples+address+bank1));
	}

	return 0;
}

#if 0
void print_led(int i)
{
	if (i)
	{
		printf("O");
	}
	else
	{
		printf("x");
	}
}
#endif

static void m2_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address < 0x80000)
	{
		scsp1_ram[address] = data;
		return;
	}

	if (address >= 0x100000 && address < 0x100fff)
	{
		address -= 0x100000;
		if (address & 1)
			SCSP_0_w(address>>1, data, 0xff00);
		else
			SCSP_0_w(address>>1, data<<8, 0x00ff);
		return;
	}

	if (address >= 0x100700 && address < 0x100f00)
		return;	// DSP write

	if (address >= 0x200000 && address < 0x280000)
	{
		scsp2_ram[address-0x200000] = data;
		return;
	}

	if (address >= 0x300000 && address < 0x300fff)
	{
		address -= 0x300000;
		if (address & 1)
			SCSP_1_w(address>>1, data, 0xff00);
		else
			SCSP_1_w(address>>1, data<<8, 0x00ff);
		return;
	}

	if (address >= 0x300700 && address < 0x300f00)
		return;	// DSP write

	if (address == 0x400001)	// diagnostic LEDs in low 4 bits
	{
#if 0
		printf("LED change: ");
		print_led((data>>3)&0x1);		
		print_led((data>>2)&0x1);		
		print_led((data>>1)&0x1);		
		print_led(data&0x1);
		printf("\n");		
#endif
		if (bankenable)
		{
			if (data & 0x20)
			{
				bank0 = 0x200000;
				bank1 = 0x600000;
			}
			else
			{
				bank0 = 0x800000;
				bank1 = 0xa00000;
			}
		}
		return;
	}
}

static void m2_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address < 0x80000)
	{
		scsp1_ram[address] = (data>>8)&0xff;
		scsp1_ram[address+1] = data&0xff;
		return;
	}

	if (address >= 0x100000 && address < 0x100fff)
	{
		SCSP_0_w((address-0x100000)>>1, data, 0x0000);
		return;
	}

	if (address >= 0x200000 && address < 0x280000)
	{
		scsp2_ram[address-0x200000] = (data>>8)&0xff;
		scsp2_ram[address-0x200000+1] = data&0xff;
		return;
	}

	if (address >= 0x300000 && address < 0x300fff)
	{
		SCSP_1_w((address-0x300000)>>1, data, 0x0000);
		return;
	}
}

static void m2_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address < 0x80000)
	{
		scsp1_ram[address] = (data>>24)&0xff;
		scsp1_ram[address+1] = (data>>16)&0xff;
		scsp1_ram[address+2] = (data>>8)&0xff;
		scsp1_ram[address+3] = data&0xff;
		return;
	}

	if (address >= 0x100000 && address < 0x100fff)
	{
		address -= 0x100000;
		SCSP_0_w(address>>1, data>>16, 0x0000);
		SCSP_0_w((address>>1)+1, data&0xffff, 0x0000);
		return;
	}

	if (address >= 0x200000 && address < 0x280000)
	{
		scsp2_ram[address-0x200000] = (data>>24)&0xff;
		scsp2_ram[address-0x200000+1] = (data>>16)&0xff;
		scsp2_ram[address-0x200000+2] = (data>>8)&0xff;
		scsp2_ram[address-0x200000+3] = data&0xff;
		return;
	}

	if (address >= 0x300000 && address < 0x300fff)
	{
		address -= 0x300000;
		SCSP_1_w(address>>1, data>>16, 0x0000);
		SCSP_1_w((address>>1)+1, data&0xffff, 0x0000);
		return;
	}

}

extern "C" {
void SCSP_TimersAddTicks(int num, int ticks);
void CheckPendingIRQ(void);
}

static void scsp_timer(int refcon)
{
//	SCSP_TimersAddTicks(0, 1);
//	CheckPendingIRQ();

	timer_set(1.0/44100.0, 0, scsp_timer);
}

static void M2_Init(long srate)
{
	bank0 = 0x200000;
	bank1 = 0x600000;
	bankenable = 0;

	bInit = 0;

	if (rom_getregionsize(RGN_SAMP1) > 0x800000)
	{
		bankenable = 1;
	}

	timer_set(1.0/44100.0, 0, scsp_timer);
}

static void M3_Init(long srate)
{
	bank0 = 0x200000;
	bank1 = 0x600000;
	bankenable = 0;

	bInit = 0;

	if (rom_getregionsize(RGN_SAMP1) > 0x800000)
	{
		bankenable = 2;
	}

	timer_set(1.0/44100.0, 0, scsp_timer);
}

static void M2_SendCmd(int cmda, int cmdb)
{
	if ((cmda == 0xff) && (Machine->refcon != 2) && (Machine->refcon != 5))
	{
		return;	// ignore "stop" command
	}

	if ((cmda == 0) && (Machine->refcon == 4)) return;

	if (Machine->refcon == 3 || Machine->refcon == 4 || Machine->refcon == 5)
	{
		SCSP_MidiIn(0, 0xa0, 0);
	}
	else
	{
		SCSP_MidiIn(0, 0xae, 0);
	}

	if (Machine->refcon == 1)
	{
		SCSP_MidiIn(0, 1, 0);
	}
	else if (Machine->refcon == 4)
	{
		SCSP_MidiIn(0, 0x11, 0);
	}
	else if ((Machine->refcon == 5) && (cmda == 0xff))
	{
		SCSP_MidiIn(0, 0, 0);
	}
	else
	{
		SCSP_MidiIn(0, 0x10|((cmda>>8)&0xf), 0);
	}

	if ((cmda == 0xff) && (Machine->refcon == 5))
	{
		SCSP_MidiIn(0, 1, 0);
	}
	else
	{
		SCSP_MidiIn(0, cmda&0xff, 0);
	}
}

M1_BOARD_START( scsp )
	MDRV_NAME("Model 2A/2B/2C")
	MDRV_HWDESC("68000, SCSP")
	MDRV_INIT( M2_Init )
	MDRV_SEND( M2_SendCmd )
	MDRV_DELAYS( 500, 15 )

	MDRV_CPU_ADD(MC68000, 11700000)
	MDRV_CPUMEMHAND(&m2_readwritemem)

	MDRV_SOUND_ADD(SCSP, &scsp_interface)
M1_BOARD_END

M1_BOARD_START( scspm3 )
	MDRV_NAME("Model 3")
	MDRV_HWDESC("68000, SCSP(x2)")
	MDRV_INIT( M3_Init )
	MDRV_SEND( M2_SendCmd )
	MDRV_DELAYS( 500, 15 )

	MDRV_CPU_ADD(MC68000, 11700000)
	MDRV_CPUMEMHAND(&m2_readwritemem)

	MDRV_SOUND_ADD(SCSP, &scsp_interface)
M1_BOARD_END

/*

srally2:
effects

FX off:
a0000000 00000000 to sound
00000000 00000000 to sound
01000000 00000000 to sound

music off:
ae000000 00000000 to sound
10000000 00000000 to sound
00000000 00000000 to sound

FX play:
a0000000 00000000 to sound
11000000 00000000 to sound
01000000 00000000 to sound (02/03/04)

voices:
a0000000 00000000 to sound
17000000 00000000 to sound
4c000000 00000000 to sound

*/
