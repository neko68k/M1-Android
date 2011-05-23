/* Williams ADPCM MK1 and friends board */

#include "m1snd.h"

#define M6809_CLOCK (2000000)
#define YM_CLOCK (3579580)

static void PCM_Init(long srate);
static void PCM_SendCmd(int cmda, int cmdb);

static unsigned int PCM_Read(unsigned int address);
static void PCM_Write(unsigned int address, unsigned int data);
static void YM2151_IRQ(int irq);
static WRITE_HANDLER( williams_dac_data_w );
static int bankofs, cmd_latch, williams_sound_int_state, adpcm_bank_count;
static int dac_enable = 0;

static M16809T cvsdrwmem =
{
	PCM_Read,
	PCM_Write,
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
	{ YM2151_IRQ }
};

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,          	/* 1 chip */
	{ 8000 },       /* 8000 Hz frequency */
	{ RGN_SAMP1 },  /* memory */
	{ 50 }
};

M1_BOARD_START( wmsadpcm )
	MDRV_NAME("Williams Y-Unit ADPCM")
	MDRV_HWDESC("6809, YM2151, MSM-6295, DAC")
	MDRV_INIT( PCM_Init )
	MDRV_SEND( PCM_SendCmd )
	MDRV_DELAYS( 1000, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&cvsdrwmem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static int ym2151_last_adr;

static void YM2151_IRQ(int irq)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( williams_dac_data_w )
{
	if (!dac_enable) return;

	DAC_data_w(0, data);
}

WRITE_HANDLER( williams_adpcm_6295_bank_select_w )
{
	if (adpcm_bank_count <= 3)
	{
		if (!(data & 0x04))
			OKIM6295_set_bank_base(0, 0x00000);
		else if (data & 0x01)
			OKIM6295_set_bank_base(0, 0x40000);
		else
			OKIM6295_set_bank_base(0, 0x80000);
	}
	else
	{
		data &= 7;
		if (data != 0)
			OKIM6295_set_bank_base(0, (data - 1) * 0x40000);
	}
}

static unsigned int PCM_Read(unsigned int address)
{
	if (address < 0x1fff)
	{
		return workram[address];
	}

	if (address >= 0x2400 && address <= 0x2401)
	{
		return YM2151ReadStatus(0);
	}

	if (address == 0x2c00)
	{
		return OKIM6295_status_0_r(0);
	}

	if (address == 0x3000)
	{
//		printf("read %x from latch\n", cmd_latch);
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		williams_sound_int_state = 0;
		return cmd_latch;
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		return prgrom[address-0x4000+bankofs];
	}

	if (address >= 0xc000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void PCM_Write(unsigned int address, unsigned int data)
{					
	if (address < 0x1fff)
	{
		workram[address] = data;
		return;
	}

	if (address == 0x2000)
	{
		int bank = data & 7;

		bankofs = 0x10000 + (bank * 0x8000);

//		printf("bankswitch: %x, %x\n", data, bankofs);
		return;
	}

	if (address == 0x2400)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x2401)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address == 0x2800)
	{
		williams_dac_data_w(0, data);
		return;
	}

	if (address == 0x2c00)
	{
		OKIM6295_data_0_w(0, data);
		return;
	}

	if (address == 0x3400)
	{
		williams_adpcm_6295_bank_select_w(0, data);
	}

	if (address == 0x3c00)
	{	
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void PCM_Init(long srate)
{
	williams_sound_int_state = 0;
	dac_enable = 0;

	adpcm_bank_count = rom_getregionsize(RGN_SAMP1) / 0x40000;

	PCM_Write(0x2000, 0);	// default to 6809 bank 0
	PCM_Write(0x3400, 0);	// also 6295 bank 0

	// install "fixed ROM"
	memcpy(&prgrom[0xc000], &prgrom[0x4c000], 0x4000);

	m1snd_addToCmdQueue(0);
}

static void write_pcm(int cmda)
{
	cmd_latch = cmda;
	if (!(cmda&0x200))
	{
		cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
		williams_sound_int_state = 1;
	}
}
/*
static void timer_stage2(int cmda)
{
	write_pcm(0x9f);
	timer_set(1.0/240.0, cmda&0xff, write_pcm);
}

static void timer_stage(int cmda)
{
	write_pcm(0x7a);
	timer_set(1.0/240.0, cmda&0xff, timer_stage2);
}

static void timer_stage0(int cmda)
{
	write_pcm(0xa3);
	timer_set(1.0/240.0, cmda&0xff, timer_stage);
}
*/
static void PCM_SendCmd(int cmda, int cmdb)
{
	dac_enable = 1;

	if (1) //cmda < 256)
	{
		write_pcm(cmda);
	}
/*	else
	{
		write_pcm(0xa5);	// prefix for voices in MK
		timer_set(1.0/240.0, cmda&0xff, timer_stage0);
	}*/
}
