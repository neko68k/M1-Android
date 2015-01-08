/* Namco System 2 and System 21 */

#include "m1snd.h"

#define M6809_CLOCK (3579580)
#define YM_CLOCK (3579580)

#define S21_TIMER_PERIOD (1.0/120.0)

static void S2x_Init(long srate);
static void S2_SendCmd(int cmda, int cmdb);
static void S21_SendCmd(int cmda, int cmdb);

static unsigned int S21_Read(unsigned int address);
static void S21_Write(unsigned int address, unsigned int data);

static int bankofs = 0, mcnt, binit;

static M16809T s21rwmem =
{
	S21_Read,
	S21_Write,
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHz ? */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ NULL }	/* YM2151 IRQ line is NOT connected on the PCB */
};

static struct C140interface c140_interface_typeA =
{
	C140_TYPE_SYSTEM21_A,
	8000000/374,
	RGN_SAMP1,
	45
};

static struct C140interface c140_interface_typeB =
{
	C140_TYPE_SYSTEM21_B,
	8000000/374,
	RGN_SAMP1,
	45
};

static struct C140interface c140_interface =
{
	C140_TYPE_SYSTEM2,
	8000000/374,
	RGN_SAMP1,
	40
};

static int ym2151_last_adr, comadr;

static void s21_timer(int refcon)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
	cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);

	// set up for next time
	timer_set(S21_TIMER_PERIOD, 0, s21_timer);
}


static unsigned int S21_Read(unsigned int address)
{
	static int bootproto[2] = { 0x6a, 0xa6 };	// see burnforc init code at 0xd030 (offset 0x1030 in the ROM)

	if (address <= 0x3fff)
	{
//		printf("read %x from bankrom at %x (%x)\n", prgrom[address + bankofs], address, address+bankofs);
		return prgrom[address + bankofs];
	}

	if (address >= 0x4000 && address <= 0x4001)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x5000 && address <= 0x51ff)
	{
		return C140_r(address&0xfff);
	}

	if (address >= 0x7000 && address <= 0x7fff)	// shared ram
	{
//		printf("read shared %x (PC=%x)\n", address - 0x7000, m6809_get_reg(REG_PC));

		// other half of boot protocol w/68k
		if ((mcnt >= 0) && (address == 0x77fd))
		{
			workram[0x10000 + address - 0x7000] = bootproto[mcnt--];
		}

		return workram[0x10000 + address - 0x7000];
	}

	if (address >= 0x7800 && address <= 0x77ff)	// shared ram mirror
	{
		return workram[0x10000 + address - 0x7800];
	}

	if (address >= 0x8000 && address <= 0x9fff)
	{
		return workram[address-0x8000];
	}

	if (address >= 0xc000)
	{
//		if (address == 0xd072) printf("%d\n", workram[0x10000 + 0x7ff]);
		return prgrom[address - 0xc000];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void S21_Write(unsigned int address, unsigned int data)
{					
	if (address == 0x4000)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x4001)
	{
//		printf("2151: %x to %x\n", data, ym2151_last_adr);
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x5000 && address <= 0x51ff)
	{	
		C140_w(address&0xfff, data);
		return;
	}

	if (address >= 0x7000 && address <=0x77ff)
	{	
		// fake the boot protocol from 68000
		if ((!binit) && (address == 0x77fe) && (data == 0xff))
		{
			binit = 1;
			mcnt = 1;
		}

		workram[0x10000 + address - 0x7000] = data;
		return;
	}

	if (address >= 0x7800 && address <=0x7fff)
	{	
		workram[0x10000 + address - 0x7800] = data;
		return;
	}

	if (address >= 0x8000 && address <= 0x9fff)
	{
		workram[address-0x8000] = data;
		return;
	}

	if (address >= 0xa000 && address <= 0xbfff)
	{
		if (address == 0xa000)
		{
			if (data == 1)
			{
				timer_set(S21_TIMER_PERIOD, 0, s21_timer);
			}
		}

		return;	// MWA_NOP in mame
	}

	if (address == 0xc001)
	{
		bankofs = ((data >> 4) & 0x0f) * 0x4000;
//		printf("bank select %x: new offset %x\n", data, bankofs);
		return;
	}

	if (address == 0xd001)
	{
		return;	// 6809 watchdog
	}

	if (address == 0xe000)	// FIRQ ack.
	{
//		printf("ACK\n");
 		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
 		cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void S2x_Init(long srate)
{
	// set appropriate bootup state for shared ram
	memset(&workram[0x10000], 0, 0x10000);
	workram[0x1003a] = 0xa6;
	workram[0x1003b] = 0;

	switch (Machine->refcon)
	{
		case 1: //GAME_CYBERSLED:
//		case GAME_SOLVALOU:
//		case GAME_LUCKYANDWILD:
//		case GAME_GOLLYGHOST:
//		case GAME_MARVELLAND:
//		case GAME_KYUUKAI:
//		case GAME_ROLLINGTHUNDER2:
//		case GAME_STEELGUNNER2:
//		case GAME_COSMOGANG:
//		case GAME_COSMOGANGJ:
//		case GAME_SWS92:
//		case GAME_SUZUKA8HOURS:
//		case GAME_SUZUKA8HOURS2:
//		case GAME_BURNINGFORCE:
//		case GAME_BUBBLETROUBLE:
//		case GAME_STEELGUNNER:
			comadr = 0x110;
			break;

		case 2: //GAME_DRAGONSABER:
			comadr = 0x111;
			break;

		case 3: //GAME_FOURTRAX:
			workram[0x105ff] = 0x65;
			comadr = 0x100;
			break;

		default:
			comadr = 0x100;
			break;
	}

	bankofs = 0;
	mcnt = -1;
	binit = 0;
}

static void S2_SendCmd(int cmda, int cmdb)
{
	if ((Machine->refcon == 4) && (cmda == 0xff))	// assault
	{
		workram[0x10000+0x506] = 1;
		return;
	}	
	if ((Machine->refcon == 5) && (cmda == 0x30))	// final lap
	{
		workram[0x10000+0x103] = 0;
		workram[0x10000+0x203] = 0;
		workram[0x10000+0x201] = 0;
		return;
	}
	if ((Machine->refcon == 6) && (cmda == 0))	// mirai ninja
	{
		workram[0x10000+0x101] = 0;
		workram[0x10000+0x201] = 0;
		return;
	}

	switch (cmda & 0xff00)
	{
		case 0:
			workram[0x10000+0xff9] = 0;
			workram[0x10000+comadr+0x100] = cmda&0xff;
			workram[0x10000+comadr+0x101] = 0x60;
			break;

		case 0x100:
			workram[0x10000+comadr+0x0] = cmda&0xff;
			workram[0x10000+comadr+0x1] = 0x60;
			break;

		case 0x200:
			workram[0x10000+comadr+0x300] = cmda&0xff;
			workram[0x10000+comadr+0x301] = 0x60;
			break;

		case 0x300:
			workram[0x10000+0xff9] = 0x80;
			workram[0x10000+comadr+0x100] = cmda&0xff;
			workram[0x10000+comadr+0x101] = 0x60;
			break;

		case 0x400:
			workram[0x10000+comadr+0x200] = cmda&0xff;
			workram[0x10000+comadr+0x201] = 0x60;
			break;
	}

}

static void S21_SendCmd(int cmda, int cmdb)
{
	switch (cmda & 0xff00)
	{
		case 0:
			workram[0x10000+0xff9] = 0;
			workram[0x10000+comadr+0x100] = cmda&0xff;
			workram[0x10000+comadr+0x101] = 0x60;
			break;

		case 0x100:
			workram[0x10000+comadr+0x0] = cmda&0xff;
			workram[0x10000+comadr+0x1] = 0x60;
			break;

		case 0x200:
			workram[0x10000+comadr+0x300] = cmda&0xff;
			workram[0x10000+comadr+0x301] = 0x60;
			break;

		case 0x300:
			workram[0x10000+0xff9] = 0x80;
			workram[0x10000+comadr+0x200] = cmda&0xff;
			workram[0x10000+comadr+0x201] = 0x60;
			break;
	}

}

M1_BOARD_START( namcos21 )
	MDRV_NAME("Namco System 21 (type A)")
	MDRV_HWDESC("M6809, YM2151, C140 (banking type S21-A)")
	MDRV_INIT( S2x_Init )
	MDRV_SEND( S21_SendCmd )
	MDRV_DELAYS( 60, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s21rwmem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(C140, &c140_interface_typeA)
M1_BOARD_END

M1_BOARD_START( namcos21b )
	MDRV_NAME("Namco System 21 (type B)")
	MDRV_HWDESC("M6809, YM2151, C140 (banking type S21-B)")
	MDRV_INIT( S2x_Init )
	MDRV_SEND( S21_SendCmd )
	MDRV_DELAYS( 60, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s21rwmem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(C140, &c140_interface_typeB)
M1_BOARD_END

M1_BOARD_START( namcos2 )
	MDRV_NAME("Namco System 2")
	MDRV_HWDESC("M6809, YM2151, C140 (banking type S2)")
	MDRV_INIT( S2x_Init )
	MDRV_SEND( S2_SendCmd )
	MDRV_DELAYS( 60, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s21rwmem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(C140, &c140_interface)
M1_BOARD_END
