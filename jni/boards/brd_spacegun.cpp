/* Taito Space Gun */
#include "m1snd.h"

static void SG_Init(long srate);
static void SG_SendCmd(int cmda, int cmdb);

UINT8 *sharedram;
UINT8 cmd_data = 0;
INT32 cmd_flag = 0;
static void *vblank_timer;

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,
	{ MIXERG(25, MIXER_GAIN_1x, MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) }
};


static unsigned int read_memory_8(unsigned int address)
{
	if (address < 0x40000)
		return prgrom[address];

	else if ( (address >= 0x20c000) && (address < 0x210000) )
		return workram[address & 0x3fff];

	else if ( (address >= 0x210000) && (address < 0x220000) )
		return sharedram[address & 0xffff];

	else if ( (address >= 0x800000) && (address < 0x800010) )
		return 0xff;

	else if ( (address == 0xc0000d) || (address == 0xe00001) )
		return 0xff;

	printf("R8 @ %x\n", address);
	return 0xff;
}

static unsigned int read_memory_16(unsigned int address)
{
	if (address < 0x40000)
		return mem_readword_swap((unsigned short *)(prgrom + address));

	else if ( (address >= 0x20c000) && (address < 0x210000) )
		return mem_readword_swap((unsigned short *)(workram + (address & 0x3fff)));

	else if ( (address >= 0x210000) && (address < 0x220000) )
		return mem_readword_swap((unsigned short *)(sharedram + (address & 0xffff)));

	else if ( ( address >= 0xc00000 ) && (address < 0xc00008) )
		return YM2610Read(0, (address / 2) & 3);

	else if ( (address >= 0x800000) && (address < 0x800010) )
		return 0xffff;

	printf("R16 @ %x\n", address);
	return 0xffff;
}

static unsigned int read_memory_32(unsigned int address)
{
	if (address < 0x40000)
		return mem_readlong_swap((unsigned int *)(prgrom + address));

	else if ( (address >= 0x20c000) && (address < 0x210000) )
		return mem_readlong_swap((unsigned int *)(workram + (address & 0x3fff)));

	else if ( (address >= 0x210000) && (address < 0x220000) )
		return mem_readlong_swap((unsigned int *)(sharedram + (address & 0xffff)));

	printf("R32 @ %x\n", address);
	return 0xffffffff;
}

static void write_memory_8(unsigned int address, unsigned int data)
{
	if ( (address >= 0x20c000) && (address < 0x210000) )
		workram[address & 0x3fff] = data;

	else if ( (address >= 0x210000) && (address < 0x220000) )
		sharedram[address & 0xffff] = data;

	else if ( (address >= 0x800000) && (address < 0x800010) )
		return;

	else if ( (address == 0xc0000d ) || (address == 0xc0000f) )
		return;

	else if ( address == 0xe00001 )
		return;

	else
		printf("W8: %x @ %x (PC=%x)\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void write_memory_16(unsigned int address, unsigned int data)
{
	if ( (address >= 0x20c000) && (address < 0x210000) )
		mem_writeword_swap((unsigned short *)(workram + (address & 0x3fff)), data);

	else if ( (address >= 0x210000) && (address < 0x220000) )
		mem_writeword_swap((unsigned short *)(sharedram + (address & 0xffff)), data);

	else if ( (address >= 0x800000) && (address < 0x800010) )
		return;

	else if ( ( address >= 0xc00000 ) && (address < 0xc00008) )
		YM2610Write(0, (address / 2) & 3, data);

	else if ( (address >= 0xc20000 ) && (address < 0xc20008) )
		return;

	else if ( address == 0xf00000 )
		return;

	else
		printf("W16: %x @ %x\n", data, address);
}

static void write_memory_32(unsigned int address, unsigned int data)
{
	if ( (address >= 0x20c000) && (address < 0x210000) )
		mem_writelong_swap((unsigned int *)(workram + (address & 0x3fff)), data);

	else if ( (address >= 0x210000) && (address < 0x220000) )
		mem_writelong_swap((unsigned int *)(sharedram + (address & 0xffff)), data);

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


M1_BOARD_START( spacegun )
	MDRV_NAME("Space Gun")
	MDRV_HWDESC("68000, YM2610")
	MDRV_INIT( SG_Init )
	MDRV_SEND( SG_SendCmd )
	MDRV_DELAYS( 500, 50 )

	MDRV_CPU_ADD(MC68000, 16000000)
	MDRV_CPUMEMHAND(&readwritemem)

	MDRV_SOUND_ADD(YM2610, &ym2610_interface)
M1_BOARD_END


static void vblank_callback(int blah)
{
	m68k_set_irq(M68K_IRQ_4);
	m68k_set_irq(M68K_IRQ_NONE);

	if (cmd_flag)
	{		
		sharedram[0x0dc1] = cmd_data;
		cmd_flag = 0;
	}
	else
		sharedram[0x0dc1] = 0xef;
}

static void SG_Init(long srate)
{	
	/* Patch some stuff */
	prgrom[0xb74] = 0x4e;
	prgrom[0xb75] = 0x71;
	prgrom[0xb9e] = 0x60;

	/* 64kB of shared RAM please */
	sharedram = (UINT8*)auto_malloc(0x10000);

	/* And a VBLANK timer */
	vblank_timer = timer_pulse(TIME_IN_HZ(60), 0, vblank_callback);
}

static void SG_SendCmd(int cmda, int cmdb)
{
	cmd_flag = 1;
	cmd_data = cmda;
}


