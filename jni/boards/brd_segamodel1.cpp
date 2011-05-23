/* -------------------------------------------------------------
 * Sega Model 1 audio memory map (CPU = 68000 at 10 MHz)
 *
 * 000000-03ffff : Program ROM (256k) 
 * c20001 : (r/w) comms to main CPU
 * c20003 : (r/w) comms to main CPU
 *
 * c40001 : MultiPCM #0
 * c40003 : 
 * c40005 : 
 *
 * c40015 : unknown
 * c40017 : unknown
 *
 * c50001 : MultiPCM #0 bank
 *
 * c60001 : MultiPCM #1
 * c60003 : 
 * c60005 : 
 *
 * c60015 : unknown
 * c60017 : unknown
 *
 * c70001 : MultiPCM #1 bank
 *
 * d00000 : YM3834 at odd addresses (there are 4 write ports and 2 read)
 *
 * f00000 : 64k Work RAM
 * 
 * IRQ assignments:
 * 2: triggered on command from host (vector to 0x180)
 * all others ignored (vector to 0x100, which is an RTE instruction)
 *
 */

#include "m1snd.h"

#define YM_CLOCK (8000000)
#define M68K_CLOCK (10000000)

static void DUSA_Init(long srate);
static void DUSA_Shutdown(void);
static void M1_SendCmd(int cmda, int cmdb);

static unsigned int m1_read_memory_8(unsigned int address);
static unsigned int m1_read_memory_16(unsigned int address);
static unsigned int m1_read_memory_32(unsigned int address);
static void m1_write_memory_8(unsigned int address, unsigned int data);
static void m1_write_memory_16(unsigned int address, unsigned int data);
static void m1_write_memory_32(unsigned int address, unsigned int data);

static unsigned char v60_comms;
static unsigned char *scsp1_ram;

static struct MultiPCM_interface m1_multipcm_interface =
{
	2,
	{ YM_CLOCK, YM_CLOCK },
	{ MULTIPCM_MODE_MODEL1, MULTIPCM_MODE_MODEL1 },
	{ (1024*1024), (1024*1024) },
	{ RGN_SAMP1, RGN_SAMP2 },
	{ YM3012_VOL(50, MIXER_PAN_LEFT, 50, MIXER_PAN_RIGHT), YM3012_VOL(50, MIXER_PAN_LEFT, 50, MIXER_PAN_RIGHT) }
};

static struct YM2612interface sys32_ym3438_interface =
{
	1,
	YM_CLOCK,
	{ MIXER(60,MIXER_PAN_LEFT), MIXER(60,MIXER_PAN_RIGHT) },
	{ 0 },	{ 0 },	{ 0 },	{ 0 }
};

static M168KT m1_readwritemem =
{
	m1_read_memory_8,
	m1_read_memory_16,
	m1_read_memory_32,
	m1_write_memory_8,
	m1_write_memory_16,
	m1_write_memory_32,
};

M1_BOARD_START( segamodel1 )
	MDRV_NAME("Model 1/2")
	MDRV_HWDESC("68000, YM3834, Sega MultiPCM(x2)")
	MDRV_DELAYS(500, 15)
	MDRV_SEND( M1_SendCmd )

	MDRV_CPU_ADD(MC68000, M68K_CLOCK)
	MDRV_CPUMEMHAND(&m1_readwritemem)

	MDRV_SOUND_ADD(YM3438, &sys32_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, &m1_multipcm_interface)
M1_BOARD_END

M1_BOARD_START( segadaytona )
	MDRV_NAME("Daytona US version")
	MDRV_HWDESC("68000, YM3834, Sega MultiPCM(x2), SCSP")
	MDRV_DELAYS(120, 15)
	MDRV_INIT( DUSA_Init )
	MDRV_SHUTDOWN( DUSA_Shutdown )
	MDRV_SEND( M1_SendCmd )

	MDRV_CPU_ADD(MC68000, M68K_CLOCK)
	MDRV_CPUMEMHAND(&m1_readwritemem)

	MDRV_SOUND_ADD(YM3438, &sys32_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, &m1_multipcm_interface)
M1_BOARD_END

static unsigned int m1_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (Machine->refcon == 1)
	{
		if (address < (512*1024))
			return scsp1_ram[address];

		if ((address >= 0x600000) && (address < 0x700000))
		{
			address &= 0x7ffff;
			return prgrom[address];
		}

		// mirror of program ROM?
		if (address >= 0x80000 && address < 0xc0000)
		{
			return prgrom[address - 0x80000];
		}
	}
	else
	{
		if (address < (256*1024))
			return prgrom[address];

		// mirror of program ROM
		if (address >= 0x80000 && address < 0xc0000)
		{
			return prgrom[address - 0x80000 + 0x20000];
		}
	}

	if (address == 0xc00001)	return 0;

	// comms with v60
	if (address == 0xc20001)
	{
//		printf("read comms: %x\n", v60_comms);
		return v60_comms;
	}

	if (address == 0xc20003)	// comms status
	{
		return 1;
	}

	// MultiPCM #1
	if (address >= 0xc40000 && address <= 0xc4ffff)
	{
		address &= 0xff;
		address >>= 1;	// make sequential (1/3/5 => 0/1/2)
		switch (address)
		{
			case 0:
				return MultiPCM_reg_0_r(0);
				break;

			default:
//				printf("Unknown MPCM #1 read at %x (PC=%08x)\n", address, m68k_get_reg(NULL, M68K_REG_PC));
				break;
		}

		return 0;
	}

	// MultiPCM #2
	if (address >= 0xc60000 && address <= 0xc6ffff)
	{
		address &= 0xff;
		address >>= 1;	// make sequential (1/3/5 => 0/1/2)
		switch (address)
		{
			case 0:
				return MultiPCM_reg_1_r(0);
				break;

			default:
//				printf("Unknown MPCM #2 read at %x (PC=%08x)\n", address, m68k_get_reg(NULL, M68K_REG_PC));
				break;
		}

		return 0;
	}

	// YM3834
	if (address == 0xd00001)
	{
		return YM2612_status_port_0_A_r(0);
	}

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0xffff;
		return workram[address];
	}

	return 0;
}

static unsigned int m1_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (Machine->refcon == 1)
	{
		if (address < (512*1024))
		{
			return mem_readword_swap((unsigned short *)(scsp1_ram+address));
		}

		if ((address >= 0x600000) && (address < 0x700000))
		{
			address &= 0x7ffff;
			return mem_readword_swap((unsigned short *)(prgrom+address));
		}
	}
	else
	{
		if (address < (256*1024))
		{
			return mem_readword_swap((unsigned short *)(prgrom+address));
		}

	}

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0xffff;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	return 0;
}

static unsigned int m1_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (Machine->refcon == 1)
	{
		if (address < 0x80000)
		{
			return mem_readlong_swap((unsigned int *)(scsp1_ram+address));
		}

		if ((address >= 0x600000) && (address < 0x700000))
		{
			address &= 0x7ffff;
			return mem_readlong_swap((unsigned int *)(prgrom+address));
		}
	}
	else
	{
		if (address < (256*1024))
		{
			return mem_readlong_swap((unsigned int *)(prgrom+address));
		}
	}

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0xffff;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

	return 0;
}

/*
static void print_led(int i)
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
*/

static void m1_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (Machine->refcon == 1)
	{
		if (address < 0x80000)
		{
			scsp1_ram[address] = data;
			return;
		}

		if (address >= 0x100000 && address < 0x100400)
		{
			return;
		}

		if (address >= 0x100400 && address < 0x100430)
		{
			return;
		}

		if (address >= 0x100700 && address < 0x100f00)
			return;	// DSP write
	}
/*
	if (address == 0xc10001)	// diagnostic LEDs in low 4 bits
	{
		printf("LED change: ");
		print_led((data>>3)&0x1);		
		print_led((data>>2)&0x1);		
		print_led((data>>1)&0x1);		
		print_led(data&0x1);
		printf("\n");		
		return;
	}
*/
	if (address == 0xc20001) {return;}	// eat writeback to v60
	if (address == 0xc20003) {return;}

	// MultiPCM #0
	if (address >= 0xc40000 && address <= 0xc4ffff)
	{
//		long origaddr = address;

		address &= 0xff;
		address >>= 1;	// make sequential (1/3/5 => 0/1/2)
		switch (address)
		{
			case 0:
			case 1:
			case 2:
				MultiPCM_reg_0_w(address, data);
				return;
				break;

			default:
//				printf("Unknown MPCM #1 write %x at %x (PC=%08x)\n", data, origaddr, m68k_get_reg(NULL, M68K_REG_PC));
				break;
		}

		return;
	}

	if (address == 0xc50001)
	{
//		printf("Set PCM 0 bank to %d\n", data);
		MultiPCM_bank_0_w(0, data);
		return;
	}

	if (address >= 0xc60000 && address <= 0xc6ffff)
	{
//		long origaddr = address;

		address &= 0xff;
		address >>= 1;	// make sequential (1/3/5 => 0/1/2)
		switch (address)
		{
			case 0:
			case 1:
			case 2:
				MultiPCM_reg_1_w(address, data);
				return;
				break;

			default:
//				printf("Unknown MPCM #2 write %x at %x (PC=%08x)\n", data, origaddr, m68k_get_reg(NULL, M68K_REG_PC));
				break;
		}

		return;
	}

	if (address == 0xc70001)
	{
//		printf("Set PCM 1 bank to %d\n", data);
		MultiPCM_bank_1_w(0, data);
		return;
	}

	// YM3834
	if (address >= 0xd00000 && address <= 0xd0ffff)
	{
		address >>= 1;
		address &= 0xf;
		switch (address)
		{
			case 0:
				YM2612_control_port_0_A_w(0, data);
				break;

			case 1:
				YM2612_data_port_0_A_w(0, data);
				break;

			case 2:
				YM2612_control_port_0_B_w(0, data);
				break;

			case 3:
				YM2612_data_port_0_B_w(0, data);
				break;
		}
		return;
	}

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0xffff;
		workram[address] = data;
		return;
	}
}

static void m1_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (Machine->refcon == 1)
	{
		if (address < 0x80000)
		{
//			mem_writeword_swap((unsigned short *)(scsp1_ram+address), data);
			return;
		}

		if (address >= 0x100000 && address < 0x100400)
		{
//			scsp_write_slot8(0, address, (data>>8)&0xff);		
//			scsp_write_slot8(0, address+1, (data)&0xff);		
			return;
		}

		if (address >= 0x100400 && address < 0x100430)
		{
//			scsp_write_reg16(0, address&0xff, data);
			return;
		}

		if (address >= 0x100700 && address < 0x100f00)
			return;	// DSP write
	}

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0xffff;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}
}

static void m1_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (Machine->refcon == 1)
	{
		if (address < 0x80000)
		{
			mem_writelong_swap((unsigned int *)(scsp1_ram+address), data);
			return;
		}

		if (address >= 0x100000 && address < 0x100400)
		{
//			scsp_write_slot8(0, address, (data>>24)&0xff);
//			scsp_write_slot8(0, address+1, (data>>16)&0xff);
//			scsp_write_slot8(0, address+2, (data>>8)&0xff);
//			scsp_write_slot8(0, address+3, (data)&0xff);
			return;
		}

		if (address >= 0x100700 && address < 0x100f00)
			return;	// DSP write
	}

	if ((address >= 0xf00000) && (address < 0xf10000))
	{
		address &= 0xffff;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}
}

static void DUSA_Init(long srate)
{
	// init the SCSP for Daytona
	scsp1_ram = (unsigned char *)malloc(512*1024);

	// copy down the vectors and stuff
	memcpy(scsp1_ram, &prgrom[(128*1024)+0x1d4], 256*1024);
}

static void DUSA_Shutdown(void)
{
	free((void *)scsp1_ram);
}

static void M1_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xff) return;	// ignore "stop" command

	if ((Machine->refcon == 1) || (Machine->refcon == 2) || (Machine->refcon == 4))
	{
		v60_comms = 0xae;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
	}

	if (Machine->refcon == 3)
	{
		v60_comms = 0xb0;
	}
	else if (Machine->refcon == 4)
	{
		v60_comms = 0x20 | (cmda & 0xf00)>>8;
	}
	else
	{
		v60_comms = 0x10;
	}
	m68k_set_irq(M68K_IRQ_2);
	m68k_execute(5000);

	v60_comms = cmda & 0xff;
	m68k_set_irq(M68K_IRQ_2);
	m68k_execute(5000);
}

