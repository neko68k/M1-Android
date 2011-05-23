/* Konami "Overdrive" */

#include "m1snd.h"

#define M6809_CLOCK (3579545)

static void OD_SendCmd(int cmda, int cmdb);

static unsigned int OD_Read(unsigned int address);
static void OD_Write(unsigned int address, unsigned int data);

static M16809T odrwmem =
{
	OD_Read,
	OD_Write,
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(90,MIXER_PAN_LEFT,90,MIXER_PAN_RIGHT) },
	{ 0 },
};

static struct K053260_interface k053260_interface =
{
	2,
	{ 3579545, 3579545 },
	{ RGN_SAMP1, RGN_SAMP1 }, /* memory region */
	{ { MIXER(60,MIXER_PAN_LEFT), MIXER(60,MIXER_PAN_RIGHT) }, { MIXER(60,MIXER_PAN_LEFT), MIXER(60,MIXER_PAN_RIGHT) } },
	{ 0, 0 }
};

static int ym2151_last_adr;

//static void YM2151_IRQ(int irq)
//{
//	printf("2151 IRQ %d\n", irq);
//}

static unsigned int OD_Read(unsigned int address)
{
	if (address == 0x201)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x400 && address <= 0x42f)
	{
		// if reading the command latch, lower IRQ
		if (address == 0x400) 
		{
			cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		}
		return K053260_0_r(address&0x3f);
	}

	if (address >= 0x600 && address <= 0x62f)
	{
		return K053260_1_r(address&0x3f);
	}

	if (address >= 0x800 && address <= 0xfff)
	{
		return workram[address-0x800];
	}

	if (address >= 0x1000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void OD_Write(unsigned int address, unsigned int data)
{					
	if (address == 0x200)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x201)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x400 && address <= 0x42f)
	{	
		K053260_0_w(address&0x3f, data);
		return;
	}

	if (address >= 0x600 && address <= 0x62f)
	{	
		K053260_1_w(address&0x3f, data);
		return;
	}

	if (address >= 0x800 && address <= 0xfff)
	{
		workram[address-0x800] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void OD_SendCmd(int cmda, int cmdb)
{
	K053260_0_w(0, cmda);	// comms go through the '260

	cpu_set_irq_line(0, M6809_IRQ_LINE, HOLD_LINE);
}

M1_BOARD_START( overdrive )
	MDRV_NAME("OverDrive")
	MDRV_HWDESC("M6809, YM2151, K053260(x2)")
	MDRV_SEND( OD_SendCmd )
	MDRV_DELAYS( 60, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&odrwmem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K053260, &k053260_interface)
M1_BOARD_END
