/*
	Namco NA-1 and NA-2

	NA-2 games can use the NA-1 BIOS for sound/music playback.
*/

#include "m1snd.h"

#define SHARED(a)		shared_ram[BYTE_XOR_BE(a)]
#define SHARED_OFFS		(0x10000)

static void NA_Init(long srate);
static void NA_SendCmd(int cmda, int cmdb);
static unsigned int na_read(unsigned int address);
static void na_write(unsigned int address, unsigned int data);
static unsigned int na_read16(unsigned int address);
static void na_write16(unsigned int address, unsigned int data);

static void *int_timer;
static UINT8 *shared_ram;
static int irq = 0;
static int bank = -1;

enum
{
	BKRTMAQ = 0,
	CGANGPZL,
	EXVANIA,
	FGHTATCK,
	EMERALDA,
	QUIZTOU,
	SWCOURT,
	TINKLPIT,
	XDAY2,
	NUMANATH,
	KNCKHEAD,
};


static const M137710T na_rw =
{
	na_read,
	na_write,
	na_read16,
	na_write16
};

static const struct C140interface c219_interface =
{
	C140_TYPE_ASIC219,
	44100,
	0,
	45
};

static unsigned int na_read(unsigned int address)
{
	if (address <= 0x7f)
	{
		return m37710_internal_r(address);
	}
	else if (address >= 0x80 && address <= 0x27f)
	{
		// 37710 internal RAM
		return workram[address];
	}
	else if (address >= 0x800 && address <= 0xfff)
	{
		// Mailbox
		return 0xff;
	}
	else if (address >= 0x1000 && address <= 0x1fff)
	{
		return C140_r((address & 0xfff) + 1);
	}
	else if (address >= 0x2000 && address <= 0x2fff)
	{
		// Shared RAM page 1 mirror
		return shared_ram[BYTE_XOR_BE(address & 0xfff)];
	}
	else if (address >= 0x3000 && address <= 0xafff)
	{
		return workram[address];
	}
	else if (address >= 0xc000 && address <= 0xffff)
	{
		return prgrom2[address - 0xc000];
	}
	else if (address >= 0x200000 && address <= 0x27ffff)
	{
		return shared_ram[BYTE_XOR_BE(address & 0x7ffff)];
	}

	return 0xff;
}

static unsigned int na_read16(unsigned int address)
{
	if (address <= 0x7f)
	{
		return m37710_internal_r(address) | m37710_internal_r(address + 1) << 8;
	}
	else if (address >= 0x80 && address <= 0x27f)
	{
		// 37710 internal RAM
		return workram[address] | workram[address + 1] << 8;
	}
	else if (address >= 0x800 && address <= 0xfff)
	{
		// Mailbox
		return 0xffff;
	}
	else if (address >= 0x1000 && address <= 0x1fff)
	{
		UINT32 offset = address & 0xfff;
		return C140_r(offset + 1) | C140_r(offset) << 8;
	}
	else if (address >= 0x2000 && address <= 0x2fff)
	{
		// Shared RAM page 1 mirror
		return (shared_ram[address & 0xfff] << 8) | shared_ram[(address & 0xfff) + 1];
	}
	else if (address >= 0x3000 && address <= 0xafff)
	{
		return (workram[address + 1] << 8) | workram[address];
	}
	else if (address >= 0xc000 && address <= 0xffff)
	{
		return (prgrom2[(address-0xc000) + 1] << 8) | prgrom2[(address-0xc000)];
	}
	else if (address >= 0x200000 && address <= 0x27ffff)
	{
		return (shared_ram[address & 0x7ffff] << 8) | shared_ram[(address & 0x7ffff) + 1];
	}
	return 0xffff;
}

static void na_write(unsigned int address, unsigned int data)
{
	if (address <= 0x7f)
	{
		m37710_internal_w(address, data);
	}
	else if (address >= 0x80 && address <= 0x27f)
	{
		workram[address] = data & 0xff;
	}
	else if (address >= 0x800 && address <= 0xfff)
	{
		// Mailbox
		return;
	}
	else if (address >= 0x1000 && address <= 0x1fff)
	{
		C140_w((address & 0xfff) + 1, data & 0xff);
	}
	else if (address >= 0x2000 && address <= 0x2fff)
	{
		// Shared RAM page 1 mirror
		shared_ram[BYTE_XOR_BE(address & 0xfff)] = data;
	}
	else if (address >= 0x3000 && address <= 0xafff)
	{
		workram[address] = data;
	}
	else if (address >= 0x200000 && address <= 0x27ffff)
	{
		shared_ram[BYTE_XOR_BE(address & 0x7ffff)] = data;
	}

}

static void na_write16(unsigned int address, unsigned int data)
{
	if (address <= 0x7f)
	{
		m37710_internal_w(address, data);
		m37710_internal_w(address + 1, (data >> 8) & 0xff);
	}
	else if (address >= 0x80 && address <= 0x27f)
	{
		workram[address] = data & 0xff;
		workram[address + 1] = (data >> 8) & 0xff;
	}
	else if (address >= 0x800 && address <= 0xfff)
	{
		// Mailbox
		return;
	}
	else if (address >= 0x1000 && address <= 0x1fff)
	{
		UINT32 offset = address & 0xfff;

		C140_w((offset) + 1, (data & 0xff));
		C140_w(offset, data >> 8);
	}
	else if (address >= 0x2000 && address <= 0x2fff)
	{
		// Shared RAM page 1 mirror
		shared_ram[address & 0xfff] = (data >> 8) & 0xff;
		shared_ram[(address & 0xfff) + 1] = data & 0xff;
	}
	else if (address >= 0x3000 && address <= 0xafff)
	{
		// Work RAM
		workram[address] = data & 0xff;
		workram[address + 1] = (data >> 8) & 0xff;
	}
	else if (address >= 0x200000 && address <= 0x27ffff)
	{
		shared_ram[address & 0x7ffff] = (data >> 8) & 0xff;
		shared_ram[(address & 0x7ffff) + 1] = data & 0xff;
	}
}

M1_BOARD_START( namcona )
	MDRV_NAME("Namco NA-1/2")
	MDRV_HWDESC("M37710, C219")
	MDRV_INIT( NA_Init )
	MDRV_SEND( NA_SendCmd )
	MDRV_DELAYS( 500, 10 )

	MDRV_CPU_ADD(M37710, 50113000/4)
	MDRV_CPUMEMHAND(&na_rw)

	MDRV_SOUND_ADD(C140, &c219_interface)
M1_BOARD_END

static void timer_callback(int ref)
{
	if (irq == 0)
	{
		cpu_set_irq_line(0, M37710_LINE_ADC, ASSERT_LINE);
		cpu_set_irq_line(0, M37710_LINE_ADC, CLEAR_LINE);
		irq = 1;
	}
	else
	{
		cpu_set_irq_line(0, M37710_LINE_IRQ1, ASSERT_LINE);
		cpu_set_irq_line(0, M37710_LINE_IRQ1, CLEAR_LINE);
		irq = 0;
	}
}

static void NA_Init(long srate)
{
	shared_ram = &workram[0x10000];
	bank = -1;

	int_timer = timer_pulse(TIME_IN_HZ(30), 0, timer_callback);
}

/* For the nice games that have lookup tables for each song... */
static const struct _entry_
{
	UINT32 samp_addr;
	UINT32 data_addr;
} game_ptrs[] = 
{
	{ 0x00000, 0x60000 },		// BKRTMAQ
	{ 0x12000, 0x62000 },		// CGANGPZL
	{ 0x40000, 0x90000 },		// EXVANIA
	{ 0x00000, 0xc0000 },		// FGHTATCK
	{ 0xa0000, 0xf0000 },		// EMERALDJ
	{ 0xa0000, 0xf0000 },		// QUIZTOU
	{ 0x48000, 0x98000 },		// SWCOURT
};


static void NA_SendCmd(int cmda, int cmdb)
{
//	int newbank;

	// First run
	if (bank == -1)
	{
		C140_set_base(shared_ram);
		
		SHARED(0x8f0) = 7;	// Metadata lives here
		SHARED(0x8f2) = 1;

		if (Machine->refcon == KNCKHEAD)
		{
			// Knuckle Heads wants metadata at 0x10000
			SHARED(0x8f0) = 1;
		}
		else if (Machine->refcon == NUMANATH)
		{
			// Don't bother doing anything
		}
		else if (Machine->refcon == TINKLPIT)
		{
			memcpy(&shared_ram[0x20000], &prgrom[0x90000],  0x20000);	// 0xc90000
			memcpy(&shared_ram[0x40000], &prgrom[0xb0000],  0x20000);	// 0xcb0000
			memcpy(&shared_ram[0x60000], &prgrom[0x3f0a40], 0x10000);   // 0x9f0a40
			memcpy(&shared_ram[0x70000], &prgrom[0xf0000],  0x10000);	// 0xdf0000
		}
		else if (Machine->refcon == XDAY2)
		{
			// Do nowt
		}
		else
		{
			// Most games have static sample data and metadata
			UINT32 samp_addr = game_ptrs[Machine->refcon].samp_addr;
			UINT32 data_addr = game_ptrs[Machine->refcon].data_addr;
			memcpy(&shared_ram[0x20000], &prgrom[samp_addr], 0x50000);
			memcpy(&shared_ram[0x70000], &prgrom[data_addr], 0x10000);
		}
		bank = 0;
	}

	if (cmda != 256)
	{
		if (Machine->refcon == KNCKHEAD)
		{
			/* Continue/game over and ending songs */
			if ((cmda == 0x31 || cmda == 0x32) || (cmda >= 0x60))
			{
				UINT32 samp1=0, samp2=0;

				memcpy(&shared_ram[0x10000], &prgrom[0x1e0000], 0x10000);
				memcpy(&shared_ram[0x60000], &prgrom[0xb00000 - 0x800000], 0x20000);

				switch (cmda)
				{
					case 0x31:
					case 0x32: samp1 = 0xb40000; samp2 = 0xbe0000; break;
					case 0x60: samp1 = 0xb20000; samp2 = 0xbe0000; break;
					case 0x61: samp1 = 0xb40000; samp2 = 0xbf0000; break;
					case 0x62: samp1 = 0xb60000; samp2 = 0xbe0000; break;
					case 0x63: samp1 = 0xb40000; samp2 = 0xbf0000; break;
					case 0x64: samp1 = 0xba0000; samp2 = 0xbf0000; break;
					case 0x65: samp1 = 0xbc0000; samp2 = 0xbf0000; break;
				}

				memcpy(&shared_ram[0x20000], &prgrom[samp1 - 0x800000], 0x20000);
				memcpy(&shared_ram[0x40000], &prgrom[samp2 - 0x800000], 0x10000);

				SHARED(0x820) = cmda;
				SHARED(0x821) = 0x40;
				return;
			}

			INT8 newbank = prgrom[BYTE_XOR_BE(0x02D34a + cmda)];

			if (newbank >= 0)
			{
				UINT32 ptr = (0x02d44a + newbank * 16);

				UINT32 addr1 = *((unsigned int *)&prgrom[WORD_XOR_BE(ptr +  0)]) - 0x800000;
				UINT32 addr2 = *((unsigned int *)&prgrom[WORD_XOR_BE(ptr +  4)]) - 0x800000;
				UINT32 addr3 = *((unsigned int *)&prgrom[WORD_XOR_BE(ptr +  8)]) - 0x800000;
				UINT32 addr4 = *((unsigned int *)&prgrom[WORD_XOR_BE(ptr + 12)]) - 0x800000;

				// Metadata
				memcpy(&shared_ram[0x10000], &prgrom[0x1f0000], 0x10000);

				memcpy(&shared_ram[0x20000], &prgrom[addr1], 0x20000);
				memcpy(&shared_ram[0x40000], &prgrom[addr2], 0x10000);
				memcpy(&shared_ram[0x50000], &prgrom[addr3], 0x10000);
				memcpy(&shared_ram[0x60000], &prgrom[addr4], 0x20000);

				SHARED(0x820) = cmda;
				SHARED(0x821) = 0x40;
			}
			return;
		}
		else if (Machine->refcon == XDAY2)
		{
			UINT32	samp1;
			UINT32	samp2;
			UINT32	samp3;
			UINT32	meta;

			switch (cmda)
			{
				case 0x1e:
				case 0x20:
				case 0x21:
				case 0x22:
				case 0x26:
				{
					samp1 = 0x320000;
					samp2 = 0x440000;
					samp3 = 0x390000;
					meta = 0xc0000;
					break;
				}
				case 0x40:
				case 0x3e:
				case 0x44:
				case 0x46:
				{
					samp1 = 0x340000;
					samp2 = 0x400000;
					samp3 = 0x3a0000;
					meta = 0xc0000;
					break;
				}
				case 0x1c:
				case 0x49:
				{
					samp1 = 0x300000;
					samp2 = 0x4a0000;
					samp3 = 0x380000;
					meta = 0xc0000;
					break;
				}
				case 0x1a:
				case 0x4c:
				{
					samp1 = 0x360000;
					samp2 = 0x460000;
					samp3 = 0x3a0000;
					meta = 0xc0000;
					break;
			   	}
				case 0x12:
				default:
				{
					samp1 = 0x320000;
					samp2 = 0x400000;
					samp3 = 0x3a0000;
					meta = 0xc0000;
	   			}
			}
			// Copy samples
			memcpy(&shared_ram[0x20000], &prgrom[samp1], 0x20000);
			memcpy(&shared_ram[0x40000], &prgrom[samp2], 0x20000);
			memcpy(&shared_ram[0x60000], &prgrom[samp3], 0x10000);

			// Copy metadata
			memcpy(&shared_ram[0x70000], &prgrom[meta], 0x10000);
		}
		else if (Machine->refcon == NUMANATH)
		{
			/*
				Numan Athletics doesn't use the correct banks during sound-test.
				Let's give it a hand...
			*/
			UINT32	samp1;
			UINT32	samp2;
			UINT32	samp3;
			UINT32	meta;

			switch (cmda)
			{
				case 0x70:
				{
					samp1 = 0xb60000;
					samp2 = 0xb40000;
					samp3 = 0xb20000;
					meta = 0xde0000;
					break;
				}
				case 0x31:
				case 0x43:
				case 0x45:
				case 0x47:
				{
					samp1 = 0xae0000;
					samp2 = 0xb00000;
					samp3 = 0xb20000;
					meta = 0xde0000;
					break;
				}
				case 0x33:
				case 0x35:
				{
					samp1 = 0xbe0000;
					samp2 = 0xb00000;
					samp3 = 0xb20000;
					meta = 0xdf0000;
					break;
				}
				case 0x37:
				{
					samp1 = 0xb60000;
					samp2 = 0xb40000;
					samp3 = 0xb30000;
					meta = 0xde0000;
					break;
				}
				case 0x75:
				{
					samp1 = 0xdc0000 - 0x500000;
					samp2 = 0xb40000;
					samp3 = 0xb30000;
					meta = 0xde0000;
					break;
				}
				case 0x7:
				case 0x9:
				case 0xf:
				case 0x11:
				case 0x13:
				case 0x15:
				case 0x39:
				case 0x3b:
				case 0x3d:
				case 0x3f:
				{
					samp1 = 0xbe0000;
					samp2 = 0xb40000;
					samp3 = 0xb30000;
					meta = 0xdf0000;
					break;
				}
				default:
				{
					samp1 = 0xbe0000;
					samp2 = 0xb00000;
					samp3 = 0xb20000;
					meta = 0xde0000;
				}
			}
			// Copy samples
			memcpy(&shared_ram[0x20000], &prgrom[samp1 - 0x800000], 0x20000);
			memcpy(&shared_ram[0x40000], &prgrom[samp2 - 0x800000], 0x20000);
			memcpy(&shared_ram[0x60000], &prgrom[samp3 - 0x800000], 0x10000);

			// Copy metadata
			memcpy(&shared_ram[0x70000], &prgrom[meta - 0xd00000], 0x10000);
		}		
		SHARED(0x820) = cmda;
		SHARED(0x821) = 0x40;
	}
	else
	{
		// Write stop command
		SHARED(0x821) = 0x00;
	}
}
