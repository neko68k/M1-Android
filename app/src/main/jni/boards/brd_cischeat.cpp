// Jaleco Big Run (68000 + YM2151 + 2xMSM-6295, varient of Mega System 1 soundboard)

#include "m1snd.h"

#define STD_FM_CLOCK	3000000
#define STD_OKI_CLOCK	  16000

static void CIS_Init(long srate);
static void CIS_SendCmd(int cmda, int cmdb);

static unsigned int cis_read_memory_8(unsigned int address);
static unsigned int cis_read_memory_16(unsigned int address);
static unsigned int cis_read_memory_32(unsigned int address);
static void cis_write_memory_8(unsigned int address, unsigned int data);
static void cis_write_memory_16(unsigned int address, unsigned int data);
static void cis_write_memory_32(unsigned int address, unsigned int data);

static int ym2151_last_adr, cmd_latch;

static M168KT cis_readwritemem =
{
	cis_read_memory_8,
	cis_read_memory_16,
	cis_read_memory_32,
	cis_write_memory_8,
	cis_write_memory_16,
	cis_write_memory_32,
};

static void YM2151_IRQ(int irq)
{
	if (irq)
	{
		m68k_set_irq(M68K_IRQ_4);	// IRQ4 on timer
	}
}

static struct YM2151interface ym2151_interface =
{
	1,
	STD_FM_CLOCK,
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{STD_OKI_CLOCK, STD_OKI_CLOCK},
	{ RGN_SAMP1, RGN_SAMP2 },
	{ MIXER(40,MIXER_PAN_CENTER), MIXER(40,MIXER_PAN_CENTER) }
};

M1_BOARD_START( cischeat )
	MDRV_NAME("Big Run")
	MDRV_HWDESC("68000, YM2151, MSM-6295(x2)")
	MDRV_INIT( CIS_Init )
	MDRV_SEND( CIS_SendCmd )

	MDRV_CPU_ADD(MC68000, 8000000)
	MDRV_CPUMEMHAND(&cis_readwritemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static unsigned int cis_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x40000)
	{
		return prgrom[address];
	}

	if (address >= 0x40000)
	{
		return cmd_latch&0xff;
	}
	if (address >= 0x40001)
	{
		return (cmd_latch>>8)&0xff;
	}

	if (address >= 0x60000)
	{
		return cmd_latch&0xff;
	}
	if (address >= 0x60001)
	{
		return (cmd_latch>>8)&0xff;
	}

	if (address >= 0x80002 && address <= 0x80003)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0xa0000 && address <= 0xa0001)
	{
		return OKIM6295_status_0_r(0);
	}

	if (address >= 0xc0000 && address <= 0xc0001)
	{
		return OKIM6295_status_1_r(0);
	}

	if (address >= 0xe0000 && address <= 0xfffff)
	{
		return workram[address-0xe0000];
	}

//	printf("Unknown read 8 at %x\n", address);
	return 0;
}

static unsigned int cis_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x40000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if (address == 0x40000)
	{
		return cmd_latch;
	}
	if (address == 0x60000)
	{
		return cmd_latch;
	}
	if (address == 0x60004)
	{
		return cmd_latch;
	}

	if (address == 0x80002)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0xa0000 && address <= 0xc0001)
	{
		return 0;	// MSM6295 status hack as per MAME
	}

	if ((address >= 0xe0000) && (address <= 0xfffff))
	{
		address -= 0xe0000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

//	printf("Unknown read 16 at %x\n", address);
	return 0;
}

static unsigned int cis_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x40000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if ((address >= 0xe0000) && (address <= 0xfffff))
	{
		address -= 0xe0000;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

//	printf("Unknown read 32 at %x\n", address);
	return 0;
}

static void cis_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x60000 && address <= 0x60001)
	{
		return;	// comms write to main cpu
	}

	if (address >= 0x80000 && address <= 0x80001)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address >= 0x80002 && address <= 0x80003)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0xa0000 && address <= 0xa0003)
	{
		OKIM6295_data_0_w(0, data);
		return;	
	}

	if (address >= 0xc0000 && address <= 0xc0003)
	{
		OKIM6295_data_1_w(0, data);
		return;	
	}

	if (address >= 0xe0000 && address < 0xfffff)
	{
		address -= 0xe0000;
		workram[address] = data;
		return;
	}

//	printf("Unknown write 8 %x to %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void cis_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	switch (Machine->refcon)
	{
		case 1:	// big run
			if (address == 0x40000)
			{
				OKIM6295_set_bank_base(0, 0x40000 * ((data >> 0) & 1) );
				OKIM6295_set_bank_base(1, 0x40000 * ((data >> 4) & 1) );
				return;
			}
			break;

		case 2:	// cisco heat
			if (address == 0x60002) return;
			if (address == 0x40002)
			{
				OKIM6295_set_bank_base(0, 0x40000 * (data & 1) ); 
				return;
			}
			if (address == 0x40004)
			{
				OKIM6295_set_bank_base(1, 0x40000 * (data & 1) ); 
				return;
			}
			break;

		case 3:	// f1 gp star
			if (address == 0x40004)
			{
				OKIM6295_set_bank_base(0, 0x40000 * (data & 1) ); 
				return;
			}
			if (address == 0x40006)
			{
				OKIM6295_set_bank_base(1, 0x40000 * (data & 1) ); 
				return;
			}
			break;
	}

	if (address == 0x60000)
	{
		return;
	}

	if (address == 0x80000)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x80002)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0xa0000 && address <= 0xa0003)
	{
		OKIM6295_data_0_w(0, data);
		return;	
	}

	if (address >= 0xc0000 && address <= 0xc0003)
	{
		OKIM6295_data_1_w(0, data);
		return;	
	}

	if (address >= 0xe0000 && address <= 0xfffff)
	{
		address -= 0xe0000;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}

//	printf("Unknown write 16 %x to %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC)); 
}

static void cis_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0xe0000 && address <= 0xfffff)
	{
		address -= 0xe0000;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}

//	printf("Unknown write 32 %x to %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC)); 
}

void phantasm_rom_decode(int cpu)
{
	data16_t	*RAM	=	(data16_t *) memory_region(RGN_CPU1+cpu);
	int i,		size	=	memory_region_length(RGN_CPU1+cpu);
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size/2 ; i++)
	{
		data16_t x,y;

		x = Endian16_Swap(RAM[i]);

// [0] def0 189a bc56 7234
// [1] fdb9 7531 eca8 6420
// [2] 0123 4567 ba98 fedc
#define BITSWAP_0	BITSWAP16(x,0xd,0xe,0xf,0x0,0x1,0x8,0x9,0xa,0xb,0xc,0x5,0x6,0x7,0x2,0x3,0x4)
#define BITSWAP_1	BITSWAP16(x,0xf,0xd,0xb,0x9,0x7,0x5,0x3,0x1,0xe,0xc,0xa,0x8,0x6,0x4,0x2,0x0)
#define BITSWAP_2	BITSWAP16(x,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc)

		if		(i < 0x08000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000/2)	{ y = BITSWAP_2; }
		else if	(i < 0x18000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000/2)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_2; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2

		RAM[i] = Endian16_Swap(y);
	}

}

void astyanax_rom_decode(int cpu)
{
	data16_t	*RAM	=	(data16_t *) memory_region(RGN_CPU1+cpu);
	int i,		size	=	memory_region_length(RGN_CPU1+cpu);
	if (size > 0x40000)	size = 0x40000;

	for (i = 0 ; i < size/2 ; i++)
	{
		data16_t x,y;

		x = Endian16_Swap(RAM[i]);

// [0] def0 a981 65cb 7234
// [1] fdb9 7531 8ace 0246
// [2] 4567 0123 ba98 fedc

#define BITSWAP_0	BITSWAP16(x,0xd,0xe,0xf,0x0,0xa,0x9,0x8,0x1,0x6,0x5,0xc,0xb,0x7,0x2,0x3,0x4)
#define BITSWAP_1	BITSWAP16(x,0xf,0xd,0xb,0x9,0x7,0x5,0x3,0x1,0x8,0xa,0xc,0xe,0x0,0x2,0x4,0x6)
#define BITSWAP_2	BITSWAP16(x,0x4,0x5,0x6,0x7,0x0,0x1,0x2,0x3,0xb,0xa,0x9,0x8,0xf,0xe,0xd,0xc)

		if		(i < 0x08000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x10000/2)	{ y = BITSWAP_2; }
		else if	(i < 0x18000/2)	{ if ( (i | (0x248/2)) != i ) {y = BITSWAP_0;} else {y = BITSWAP_1;} }
		else if	(i < 0x20000/2)	{ y = BITSWAP_1; }
		else 					{ y = BITSWAP_2; }

#undef	BITSWAP_0
#undef	BITSWAP_1
#undef	BITSWAP_2

		RAM[i] = Endian16_Swap(y);
	}
}

static void CIS_Init(long srate)
{
	unsigned char *ROM = memory_region(RGN_CPU1), t;
	int i;

	switch (Machine->refcon)
	{
		case 2:
			astyanax_rom_decode(0);
			for (i = 0; i < 0x40000; i += 2)
			{
				t = ROM[i];
				ROM[i] = ROM[i+1];
				ROM[i+1] = t;
			}
			break;

		case 1:
			phantasm_rom_decode(0);
			for (i = 0; i < 0x40000; i += 2)
			{
				t = ROM[i];
				ROM[i] = ROM[i+1];
				ROM[i+1] = t;
			}

			// sample roms need help too
			ROM = memory_region(RGN_SAMP1);
			memcpy(&ROM[0x00000], &ROM[0x80000], 0x20000);
			memcpy(&ROM[0x40000], &ROM[0xa0000], 0x20000);
			memcpy(&ROM[0x20000], &ROM[0xc0000], 0x20000);
			memcpy(&ROM[0x60000], &ROM[0xe0000], 0x20000);

			ROM = memory_region(RGN_SAMP2);
			memcpy(&ROM[0x00000], &ROM[0x80000], 0x20000);
			memcpy(&ROM[0x40000], &ROM[0xa0000], 0x20000);
			memcpy(&ROM[0x20000], &ROM[0xc0000], 0x20000);
			memcpy(&ROM[0x60000], &ROM[0xe0000], 0x20000);
			break;
	}
}

static void CIS_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;	
}
