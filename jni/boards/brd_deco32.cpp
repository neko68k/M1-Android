/* later-model DECO Hu6280 based soundboards */
/* first rev (cbustr): Hu6280 + YM2151 + YM2203 + MSM6295(x2) */
/* later rev (sbtime): Hu6280 + YM2151 + MSM6295.
   program tries to init 2203 and second 6295 but they physically aren't
   on the board! */
/* later rev (deco32): Hu6280 + YM2151 + MSM6295(x2) */

/* Note: Dragon Gun reportedly has 3 MSM-6295s and indeed it has 3 ROMs for them, 
   but there are no unmapped writes for any of the commands... */

#include "m1snd.h"

#define H6280_CLOCK (32220000/8)
#define YM_CLOCK  (32220000/9) 
#define YM2203_CLOCK (32220000/8)
#define OKI_CLOCK (32220000/32/132)
#define OKI2_CLOCK (32220000/16/132)

static void Sly_Init(long srate);
static void Dec32_Update(long dsppos, long dspframes);
static void Dec32_SendCmd(int cmda, int cmdb);

static int ym2151_last_adr, cmd_latch;

static unsigned int dec32_read(unsigned int address);
static void dec32_write(unsigned int address, unsigned int data);
static unsigned int sly_read(unsigned int address);
static void sly_write(unsigned int address, unsigned int data);
static unsigned int mres_read(unsigned int address);
static void mres_write(unsigned int address, unsigned int data);
static void dec32_writeport(unsigned int address, unsigned int data);
static void sound_irq(int irq);
static WRITE_HANDLER( sound_bankswitch_w );

M16280T dec32_rw =
{
	dec32_read,
	dec32_write,
	dec32_writeport,
};

M16280T sly_rw =
{
	sly_read,
	sly_write,
	dec32_writeport,
};

M16280T mid_rw =
{
	mres_read,
	mres_write,
	dec32_writeport,
};

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct YM2203interface ym2203_interface =
{
	1,
	32220000/8, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2203interface sly_ym2203_interface =
{
	1,
	1500000,	/* 12MHz clock divided by 8 = 1.50 MHz */
	{ YM2203_VOL(35,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812b_interface =
{
	1,			/* 1 chip */
	3000000,
	{ 80 },
	{ sound_irq },
};

static struct OKIM6295interface sly_okim6295_interface =
{
	1,                  /* 1 chip */
	{ 7757 },           /* 8000Hz frequency */
	{ RGN_SAMP1 },		/* memory region */
	{ 80 }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ RGN_SAMP1, RGN_SAMP2 },
	{ 45, 15 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

static struct OKIM6295interface sbtokim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ RGN_SAMP1 },	/* memory region */
	{ 50 }
};

static struct YM2151interface mmym2151_interface =
{
	1,
	21470000/6, /* ?? Audio section crystal is 21.470 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct OKIM6295interface mmokim6295_interface =
{
	2,              /* 2 chips */
	{ 7757, 15514 },/* ?? Frequency */
	{ RGN_SAMP1, RGN_SAMP2 },	/* memory regions */
	{ 50, 25 }		/* Note!  Keep chip 1 (voices) louder than chip 2 */
};

static struct YM2203interface mmym2203_interface =
{
	1,
	21470000/6,	/* ?? Audio section crystal is 21.470 MHz */
	{ YM2203_VOL(40,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

M1_BOARD_START( deco32 )
	MDRV_NAME("DECO32")
	MDRV_HWDESC("Hu6280, YM2151, MSM-6295(x2)")
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( Dec32_SendCmd )

	MDRV_CPU_ADD(HU6280, H6280_CLOCK)
	MDRV_CPUMEMHAND(&dec32_rw)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START( cbuster )
	MDRV_NAME("Crude Buster")
	MDRV_HWDESC("Hu6280, YM2203, YM2151, MSM-6295(x2)")
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( Dec32_SendCmd )

	MDRV_CPU_ADD(HU6280, H6280_CLOCK)
	MDRV_CPUMEMHAND(&dec32_rw)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START( slyspy )
	MDRV_NAME("Sly Spy")
	MDRV_HWDESC("Hu6280, YM2203, YM3812, MSM-6295")
	MDRV_INIT( Sly_Init )
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( Dec32_SendCmd )

	MDRV_CPU_ADD(HU6280, H6280_CLOCK)
	MDRV_CPUMEMHAND(&sly_rw)

	MDRV_SOUND_ADD(YM2203, &sly_ym2203_interface)
	MDRV_SOUND_ADD(YM3812, &ym3812b_interface)
	MDRV_SOUND_ADD(OKIM6295, &sly_okim6295_interface)
M1_BOARD_END

M1_BOARD_START( midres )
	MDRV_NAME("Midnight Resistance")
	MDRV_HWDESC("Hu6280, YM2203, YM3812, MSM-6295")
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( Dec32_SendCmd )

	MDRV_CPU_ADD(HU6280, H6280_CLOCK)
	MDRV_CPUMEMHAND(&mid_rw)

	MDRV_SOUND_ADD(YM2203, &sly_ym2203_interface)
	MDRV_SOUND_ADD(YM3812, &ym3812b_interface)
	MDRV_SOUND_ADD(OKIM6295, &sly_okim6295_interface)
M1_BOARD_END

M1_BOARD_START( madmotor )
	MDRV_NAME("Mad Motor")
	MDRV_HWDESC("Hu6280, YM2203, YM2151, MSM-6295(x2)")
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( Dec32_SendCmd )

	MDRV_CPU_ADD(HU6280, H6280_CLOCK)
	MDRV_CPUMEMHAND(&dec32_rw)

	MDRV_SOUND_ADD(YM2151, &mmym2151_interface)
	MDRV_SOUND_ADD(YM2203, &mmym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &mmokim6295_interface)
M1_BOARD_END

M1_BOARD_START( superbtime )
	MDRV_NAME("Super BurgerTime")
	MDRV_HWDESC("Hu6280, YM2151, MSM-6295")
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( Dec32_SendCmd )

	MDRV_CPU_ADD(HU6280, H6280_CLOCK)
	MDRV_CPUMEMHAND(&dec32_rw)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &sbtokim6295_interface)
M1_BOARD_END

static void dec32_writeport(unsigned int address, unsigned int data)
{
//	printf("6280 port write %x at %x\n", data, address);
}

static unsigned int dec32_read(unsigned int address)
{
	if (address < 0xffff) return prgrom[address];
	if (address >= 0x100000 && address <= 0x100001) return YM2203Read(0, address&0x1);
	if (address >= 0x110000 && address <= 0x110001) return YM2151ReadStatus(0);
	if (address >= 0x120000 && address <= 0x120001) return OKIM6295_status_0_r(0);
	if (address >= 0x130000 && address <= 0x130001) return OKIM6295_status_1_r(0);
	if (address >= 0x140000 && address <= 0x140001)
	{
//		printf("Reading %x from cmd latch\n", cmd_latch);
	 	h6280_set_irq_line(0, CLEAR_LINE);	// Hu6280 IRQ0
		return cmd_latch;
	}

	if (address >= 0x1f0000 && address <= 0x1f1fff) return workram[address-0x1f0000];
	if (address >= 0x1fec00 && address <= 0x1fec01)
	{
		return H6280_timer_r(address&0x1);
	}
	if (address >= 0x1ff402 && address <= 0x1ff403)
	{
		return H6280_irq_status_r(address - 0x1ff402);
	}

//	printf("Unknown read at %x\n", address);
	return 0;
}

static void dec32_write(unsigned int address, unsigned int data)
{
	if (address >= 0x100000 && address <= 0x100001)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}
	if (address == 0x110000)
	{
		ym2151_last_adr = data;
		return;
	}
	if (address == 0x110001)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}
	if (address >= 0x120000 && address <= 0x120001)
	{
//		printf("%x to OKI 0 at %x\n", data, address);
		OKIM6295_data_0_w(0, data);
		return;
	}
	if (address >= 0x130000 && address <= 0x130001)
	{
//		printf("%x to OKI 1 at %x\n", data, address);
		OKIM6295_data_1_w(0, data);
		return;
	}
	if (address >= 0x1f0000 && address <= 0x1f1fff)
	{
		workram[address-0x1f0000] = data;
		return;
	}
	if (address >= 0x1fec00 && address <= 0x1fec01)
	{
		H6280_timer_w(address&0x1, data);
		return;
	}
	if (address >= 0x1ff402 && address <= 0x1ff403)
	{
		H6280_irq_status_w(address - 0x1ff402, data);
		return;
	}

//	printf("Unknown write %x to %x\n", data, address);
}

static unsigned int sly_read(unsigned int address)
{
	if (address < 0xffff) return prgrom[address];
	if (address >= 0xa0000 && address <= 0xa0001) return 0;
	if (address >= 0xb0000 && address <= 0xb0001) return YM2203Read(0, address&0x1);
	if (address >= 0xe0000 && address <= 0xe0001) return OKIM6295_status_0_r(0); 
	if (address >= 0xf0000 && address <= 0xf0001)
	{
//		printf("Reading %x from cmd latch\n", cmd_latch);
	 	h6280_set_irq_line(0, CLEAR_LINE);	// Hu6280 IRQ0
		return cmd_latch;
	}

	if (address >= 0x1f0000 && address <= 0x1f1fff) return workram[address-0x1f0000];
	if (address >= 0x1fec00 && address <= 0x1fec01)
	{
		return H6280_timer_r(address&0x1);
	}
	if (address >= 0x1ff402 && address <= 0x1ff403)
	{
		return H6280_irq_status_r(address - 0x1ff402);
	}

//	printf("Unknown read at %x\n", address);
	return 0;
}

static void sly_write(unsigned int address, unsigned int data)
{
	if (address == 0x90000)
	{
		YM3812_control_port_0_w(0, data);
		return;
	}
	if (address == 0x90001)
	{
		YM3812_write_port_0_w(0, data);
		return;
	}
	if (address >= 0xb0000 && address <= 0xb0001)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}
	if (address >= 0xe0000 && address <= 0xe0001)
	{
//		printf("%x to OKI 0 at %x\n", data, address);
		OKIM6295_data_0_w(0, data);
		return;
	}
	if (address >= 0x1f0000 && address <= 0x1f1fff)
	{
		workram[address-0x1f0000] = data;
		return;
	}
	if (address >= 0x1fec00 && address <= 0x1fec01)
	{
		H6280_timer_w(address&0x1, data);
		return;
	}
	if (address >= 0x1ff402 && address <= 0x1ff403)
	{
		H6280_irq_status_w(address - 0x1ff402, data);
		return;
	}

//	printf("Unknown write %x to %x\n", data, address);
}

static unsigned int mres_read(unsigned int address)
{
	if (address < 0xffff) return prgrom[address];
	if (address >= 0x130000 && address <= 0x130001) return OKIM6295_status_0_r(0); 
	if (address >= 0x138000 && address <= 0x138001)
	{
//		printf("Reading %x from cmd latch\n", cmd_latch);
	 	h6280_set_irq_line(0, CLEAR_LINE);	// Hu6280 IRQ0
		return cmd_latch;
	}
	if (address >= 0x1f0000 && address <= 0x1f1fff) return workram[address-0x1f0000];
	if (address >= 0x1fec00 && address <= 0x1fec01)
	{
		return H6280_timer_r(address&0x1);
	}
	if (address >= 0x1ff402 && address <= 0x1ff403)
	{
		return H6280_irq_status_r(address - 0x1ff402);
	}

//	printf("Unknown read at %x\n", address);
	return 0;
}

static void mres_write(unsigned int address, unsigned int data)
{
	if (address == 0x108000)
	{
		YM3812_control_port_0_w(0, data);
		return;
	}
	if (address == 0x108001)
	{
		YM3812_write_port_0_w(0, data);
		return;
	}
	if (address >= 0x118000 && address <= 0x118001)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}
	if (address >= 0x130000 && address <= 0x130001)
	{
//		printf("%x to OKI 0 at %x\n", data, address);
		OKIM6295_data_0_w(0, data);
		return;
	}
	if (address >= 0x1f0000 && address <= 0x1f1fff)
	{
		workram[address-0x1f0000] = data;
		return;
	}
	if (address >= 0x1fec00 && address <= 0x1fec01)
	{
		H6280_timer_w(address&0x1, data);
		return;
	}
	if (address >= 0x1ff402 && address <= 0x1ff403)
	{
		H6280_irq_status_w(address - 0x1ff402, data);
		return;
	}

//	printf("Unknown write %x to %x\n", data, address);
}

static void sound_irq(int irq)
{
//	printf("YM IRQ %d\n", irq);

	if (irq)
	{
		h6280_set_irq_line(1, ASSERT_LINE);	// Hu6280 IRQ2
	}
	else
	{
		h6280_set_irq_line(1, CLEAR_LINE);
	}
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(0, ((data >> 0)& 1) * 0x40000);
	OKIM6295_set_bank_base(1, ((data >> 1)& 1) * 0x40000);

//	printf("6295 bank: %x\n", data);
}

static void Sly_Init(long srate)
{
	int i;

	// decrypt and patch protection
	for (i = 0; i < 65536; i++)
	{
		prgrom[i]=(prgrom[i]&0x7e) | ((prgrom[i]&1)<<7) | ((prgrom[i]&0x80)>>7);
	}

	prgrom[0xf2d] = 0xea;
	prgrom[0xf2e] = 0xea;
}

// subclass the stream system :)
static void Dec32_Update(long dsppos, long dspframes)
{
	ym2151_hack = 2.0;

	StreamSys_Update(dsppos, dspframes);
}

static void Dec32_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
 	h6280_set_irq_line(0, ASSERT_LINE);	// Hu6280 IRQ0
}
