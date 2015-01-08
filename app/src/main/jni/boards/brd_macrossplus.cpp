// Macross Plus
//
// all IRQs are hooked.
//
// Memory map:
// 000000-0FFFFF : Program ROM (1 meg)
// 200000-207FFF : Work RAM (32k)
// 400000-40007F : Ensoniq ES5506
// 600000        : Command latch from main CPU (16 bits wide)
//
// source 1 = timer? ES5506?
// source 2 = command
// source 3 = ES5506

#include "m1snd.h"

#define M68K_CLOCK   (16000000)
#define ES5506_CLOCK (16000000)

static void MP_Init(long srate);
static void MP_SendCmd(int cmda, int cmdb);

#if 0
static int initstr[] =
{
#if 0
	0x0,
	0xfe03,
	0x204,
	0x5,
	0xfe13,
	0x214,
	0x15,
	0xfe23,
	0x224,
	0x25,
	0xfe33,
	0x234,
	0x35,
	0xfe43,
	0x244,
	0x45,
	0xfe53,
	0x254,
	0x55,
	0xfe63,
	0x264,
	0x65,
	0xfe73,
	0x274,
	0x75,
	0xfe83,
	0x284,
	0x85,
	0xfe93,
	0x294,
	0x95,
	0xfea3,
	0x2a4,
	0xa5,
	0,
	0,
#else
0x0 ,
0xfe03 ,
0x204 ,
0x5 ,
0xfe13 ,
0x214 ,
0x15 ,
0xfe23 ,
0x224 ,
0x25 ,
0xfe33 ,
0x234 ,
0x35 ,
0xfe43 ,
0x244 ,
0x45 ,
0xfe53 ,
0x254 ,
0x55 ,
0xfe63 ,
0x264 ,
0x65 ,
0xfe73 ,
0x274 ,
0x75 ,
0xfe83 ,
0x284 ,
0x85 ,
0xfe93 ,
0x294 ,
0x95 ,
0xfea3 ,
0x2a4 ,
0xa5 ,
0xfeb3 ,
0x2b4 ,
0xb5 ,
0xfec3 ,
0x2c4 ,
0xc5 ,
0xfed3 ,
0x2d4 ,
0xd5 ,
0xfee3 ,
0x2e4 ,
0xe5 ,
0xfef3 ,
0x2f4 ,
0xf5 ,
0x0 ,
0x0 ,
0x5 ,
0x204 ,
0xfe03 ,
0x1 ,
#endif
	-1
};
#endif

static unsigned int mp_read_memory_8(unsigned int address);
static unsigned int mp_read_memory_16(unsigned int address);
static unsigned int mp_read_memory_32(unsigned int address);
static void mp_write_memory_8(unsigned int address, unsigned int data);
static void mp_write_memory_16(unsigned int address, unsigned int data);
static void mp_write_memory_32(unsigned int address, unsigned int data);
static void es5506_irq(int irq);

static int cmd_latch, cmd_ack;

static M168KT mp_readwritemem =
{
	mp_read_memory_8,
	mp_read_memory_16,
	mp_read_memory_32,
	mp_write_memory_8,
	mp_write_memory_16,
	mp_write_memory_32,
};

static struct ES5506interface es5506_interface =
{
	1,
	{ 16000000 },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ RGN_SAMP3 },
	{ RGN_SAMP4 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ es5506_irq }
};

static unsigned int mp_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x100000)
	{
		return prgrom[address];
	}

	if (address >= 0x200000 && address <= 0x207fff)
	{
		return workram[address-0x200000];
	}

//	printf("Unknown read 8 at %x\n", address);

	return 0;
}

static unsigned int mp_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x100000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if ((address >= 0x200000) && (address <= 0x207fff))
	{
		address -= 0x200000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address >= 0x400000 && address <= 0x40007f)
	{
		address &= 0x7f;
		address >>= 1;
		return ES5506_data_0_word_r(address, 0);
	}

	if (address == 0x600000)
	{
//		printf("read %x from cmd_latch\n", cmd_latch);

		cmd_ack = 1;

		return cmd_latch;
	}

//	printf("Unknown read 16 at %x\n", address);
	return 0;
}

static unsigned int mp_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x100000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if ((address >= 0x200000) && (address <= 0x207fff))
	{
		address -= 0x200000;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

//	printf("Unknown read 32 at %x\n", address);
	return 0;
}

static void mp_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x200000 && address < 0x207fff)
	{
		address -= 0x200000;
		workram[address] = data;
		return;
	}

//	printf("Unknown write 8 %x to %x\n", data, address);
}

static void mp_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x200000 && address <= 0x207fff)
	{
		address -= 0x200000;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}

	if (address >= 0x400000 && address <= 0x400460)
	{
		address &= 0xfff;
		address >>= 1;
//		printf("ES5506: %x at %x\n", data, address);
		ES5506_data_0_word_w(address, data, 0);
		return;
	}

//	printf("Unknown write 16 %x to %x\n", data, address);
}

static void mp_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x200000 && address <= 0x207fff)
	{
		address -= 0x200000;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}

//	printf("Unknown write 32 %x to %x\n", data, address);
}

static void es5506_irq(int irq)
{
//	printf("5506 IRQ: %d\n", irq);
 	m68k_set_irq(M68K_IRQ_1);
}

static void MP_Init(long srate)
{
	data8_t* src=memory_region(REGION_SOUND1);
	data8_t* dst=memory_region(REGION_SOUND2);

	memcpy(dst,src+0x400000,0x400000);

	src=memory_region(REGION_SOUND3);
	dst=memory_region(REGION_SOUND4);

	if (src && dst)
	{
		memcpy(dst,src+0x400000,0x400000);
	}

	m1snd_addToCmdQueue(0x7fffffff);

	timer_setnoturbo();
}

static void MP_SendCmd(int cmda, int cmdb)
{
//	int i = 0;

//	cmd_latch = cmda>>8 | (cmda&0xff)<<8;	

 //	if (cmd_latch == 0x0500)
//		m68k_set_irq(M68K_IRQ_2);

	if (cmda == 0xffff)	// stop
	{
		cmd_latch = 0;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		return;
	}

	if (cmda >= 0 && cmda < 0x100)	// music bank
	{
		cmd_latch = 5;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		cmd_latch = 0x204;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		cmd_latch = 0xfe03;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		cmd_latch = cmda<<9 | 0x01;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
	}

	if (cmda >= 0x100)	// SFX bank
	{
		cmd_latch = 0x14;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		cmd_latch = 0xfe13;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		cmd_latch = 0x15;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
		cmd_latch = cmda<<9 | 0x11;
		m68k_set_irq(M68K_IRQ_2);
		m68k_execute(5000);
	}

	if (cmda == 0x7fffffff)
	{
//		printf("init str\n");

		cmd_ack = 0;
		cmd_latch = 0;
		while (!cmd_ack)
		{
			m68k_set_irq(M68K_IRQ_2);
			m68k_execute(5000);
		}
#if 0
//		printf("init2\n");
		i = 0;
		while (initstr[i] != -1)
		{
			cmd_latch = initstr[i++];
			m68k_set_irq(M68K_IRQ_2);
			m68k_execute(5000);
		}
#endif
	}
}

M1_BOARD_START( macrossplus )
	MDRV_NAME( "Macross Plus" )
	MDRV_HWDESC( "68000, ES5506" )
	MDRV_DELAYS( 1000, 50 )
	MDRV_INIT( MP_Init )
	MDRV_SEND( MP_SendCmd )

	MDRV_CPU_ADD(MC68000, M68K_CLOCK)
	MDRV_CPUMEMHAND(&mp_readwritemem)

	MDRV_SOUND_ADD(ES5506, &es5506_interface)
M1_BOARD_END

