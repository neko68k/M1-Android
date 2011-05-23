/* Namco System 1 */

#include "m1snd.h"

#define M6809_CLOCK (49152000/32)
#define YM_CLOCK (3579580)

#define S1_TIMER_PERIOD (1.0/60.0)

static void S1_Init(long srate);
static void S1_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);

static unsigned int S1_Read(unsigned int address);
static void S1_Write(unsigned int address, unsigned int data);

static int bankofs = 0, chkackadrs, cbufsize, bInit;
static unsigned int chkstatusadrs;

static struct YM2151interface ym2151_interface =
{
	1,          /* 1 chip */
	3579580,    /* 3.58 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	24000/2,	/* sample rate (approximate value) */
	8,		/* number of voices */
	75, 		/* playback volume */
	-1, 		/* memory region */
	1		/* stereo */
};

static M16809T s1rwmem =
{
	S1_Read,
	S1_Write,
};

M1_BOARD_START( namcos1 )
	MDRV_NAME("Namco System 1")
	MDRV_HWDESC("M6809, YM2151, Namco WSG")
	MDRV_INIT( S1_Init )
	MDRV_SEND( S1_SendCmd )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s1rwmem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

static int ym2151_last_adr;

static void YM2151_IRQ(int irq)
{
//	printf("2151 FIRQ: %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}

static unsigned int S1_Read(unsigned int address)
{
	if (address <= 0x3fff)
	{
//		printf("read %x from bankrom at %x (%x)\n", prgrom[address + bankofs], address, address+bankofs);
		return prgrom[address + bankofs];
	}

	if (address >= 0x4000 && address <= 0x4001)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x4100 && address <= 0x4fff)
	{
		return workram[address];
	}

	if (address >= 0x5000 && address <= 0x50ff)
	{
//		printf("read at %x (PC=%x)\n", address, m6809_get_reg(REG_PC));
		return namcos1_wavedata_r(address-0x5000);
	}

	if (address >= 0x5100 && address <= 0x513f)
	{
		return namcos1_sound_r(address-0x5100);
	}

	if (address >= 0x5140 && address <= 0x54ff)
	{
		return workram[address];
	}

	if (address >= 0x6000 && address <= 0x6fff)
	{
		return workram[address];
	}

	if (address >= 0x7000 && address <= 0x77ff)
	{
		return workram[address];
	}

	if (address >= 0x8000 && address <= 0x9fff)
	{
		return workram[address];
	}

	if (address >= 0xc000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void S1_Write(unsigned int address, unsigned int data)
{					
//	if (address != 0xd001)
//		printf("write %x at %x\n", data, address);
	
	if ((!bInit) && (address == chkstatusadrs))
	{
		workram[chkackadrs] = data;
		bInit = 1;
		return;
	}

	if ((address == 0xc001) || (address == 0xc000))
	{
		bankofs = ((data >> 4) & 0x07) * 0x4000;
		bankofs += 0xc000;
//		printf("bank select %x: new offset %x\n", data, bankofs);
		return;
	}

	if (address == 0x4000)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x4001)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x4100 && address <=0x4fff)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x5000 && address <= 0x50ff)
	{	
		namcos1_wavedata_w(address-0x5000, data);
		return;
	}

	if (address >= 0x5100 && address <= 0x513f)
	{	
		namcos1_sound_w(address-0x5100, data);
		return;
	}

	if (address >= 0x5140 && address <=0x54ff)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x6000 && address <=0x6fff)
	{
		workram[address] = data;
		return;
	}

	if (address >= 0x7000 && address <=0x77ff)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x8000 && address <= 0x9fff)
	{
		workram[address] = data;
		return;
	}

	if (address == 0xd001)
	{
//		printf("watchdog\n");
		return;	// 6809 watchdog
	}

	if (address == 0xe000)
	{
//		printf("clear IRQ\n");
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		return;	// hmm
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void s1_timer(int refcon)
{
	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);

	// set up for next time
	timer_set(S1_TIMER_PERIOD, 0, s1_timer);
}

static void S1_Init(long srate)
{
	bankofs = 0;

	namco_wavedata = &workram[0x5000];
	namco_soundregs = &workram[0x5100];

	cbufsize = 64;
	bInit = 0;

	timer_set(S1_TIMER_PERIOD, 0, s1_timer);

	switch (Machine->refcon)
	{
		case 1: //GAME_SHADOWLAND:
//		case GAME_BERABOHMAN:
//		case GAME_MARCHENMAZE:
//		case GAME_BLASTOFF:
			chkstatusadrs = 0xffffff;
			chkackadrs = -1;
			bInit = 1;
			workram[0x7000] = 0xa6;
			workram[0x7001] = 0;
			break;
		
		default:
			chkstatusadrs = 0x5002;
			chkackadrs = 0x5000;
			workram[0x5000] = 0x00;
			workram[0x5001] = 0xa6;
			break;
	}
}

static void S1_SendCmd(int cmda, int cmdb)
{
	int i, cclamp;

	if (cmda < 256)
	{
		workram[0x7100] = cmda;
		workram[0x7101] = 0x60;
	}
	else
	{
		cclamp = cmda & 0xff;
		workram[0x5240 + cclamp] = 1;
	}

	if (cmda == games[curgame].stopcmd)
	{
		if (Machine->refcon == 2)	// quester
		{
			workram[0x7101] = 0;
		}

		for (i = 0; i < cbufsize; i++)
		{
			if (workram[0x5240+i])
			{
				workram[0x5240+i] = 0;
				workram[0x5240+cbufsize+i] = 1;
			}
		}
	}
}
