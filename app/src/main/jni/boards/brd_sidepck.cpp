/* DECO Side Pocket (6502 + 2203 + 3526) */
/* DECO Karnov/Chelnov (6502 + 2203 + 3526) */
/* DECO Robocop and pals (6502 + 2203 + 3812 + 6295) */

#include "m1snd.h"

#define M6502_CLOCK (1500000)

static void SIDEP_SendCmd(int cmda, int cmdb);
static void OPLIRQ(int irq);

static unsigned int karnov_readmem(unsigned int address);
static unsigned int karnov_readop(unsigned int address);
static void karnov_writemem(unsigned int address, unsigned int data);
static unsigned int sidep_readmem(unsigned int address);
static unsigned int sidep_readop(unsigned int address);
static void sidep_writemem(unsigned int address, unsigned int data);
static unsigned int dec0_readmem(unsigned int address);
static unsigned int dec0_readop(unsigned int address);
static void dec0_writemem(unsigned int address, unsigned int data);
static unsigned int dec1_readmem(unsigned int address);
static unsigned int dec1_readop(unsigned int address);
static void dec1_writemem(unsigned int address, unsigned int data);

static int cmd_latch;

static M16502T btrw = 
{
	sidep_readmem,
	sidep_readop,
	sidep_writemem,
};

static M16502T karnovrw = 
{
	karnov_readmem,
	karnov_readop,
	karnov_writemem,
};

static M16502T d0rw = 
{
	dec0_readmem,
	dec0_readop,
	dec0_writemem,
};

static M16502T d1rw = 
{
	dec1_readmem,
	dec1_readop,
	dec1_writemem,
};

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* Should be accurate for all games, derived from 12MHz crystal */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2203interface lm_ym2203_interface =
{
	1,
	1500000,	/* Should be accurate for all games, derived from 12MHz crystal */
	{ YM2203_VOL(90,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz */
	{ 90 },		/* volume */
	{ OPLIRQ },
};

static struct YM3812interface ym3812_interface =
{
	1,		/* 1 chip */
	3000000,	/* 3 MHz */
	{ 70 },
	{ OPLIRQ },
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 7757 },           /* 8000Hz frequency */
	{ RGN_SAMP1 },	/* memory region */
	{ 80 }
};

M1_BOARD_START( sidepocket )
	MDRV_NAME("Side Pocket")
	MDRV_HWDESC("6502, YM2203, YM3526")
	MDRV_DELAYS(1000, 15)
	MDRV_SEND( SIDEP_SendCmd )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&btrw)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
M1_BOARD_END

M1_BOARD_START( karnov )
	MDRV_NAME("Karnov")
	MDRV_HWDESC("6502, YM2203, YM3526")
	MDRV_DELAYS(1000, 15)
	MDRV_SEND( SIDEP_SendCmd )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&karnovrw)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
M1_BOARD_END

M1_BOARD_START( dec0 )
	MDRV_NAME("Robocop")
	MDRV_HWDESC("6502, YM2203, YM3812, MSM-6295")
	MDRV_DELAYS(1000, 15)
	MDRV_SEND( SIDEP_SendCmd )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&d0rw)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START( dec1 )
	MDRV_NAME("Last Mission")
	MDRV_HWDESC("6502, YM2203, YM3526")
	MDRV_DELAYS(1000, 15)
	MDRV_SEND( SIDEP_SendCmd )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&d1rw)

	MDRV_SOUND_ADD(YM2203, &lm_ym2203_interface)
	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
M1_BOARD_END

static unsigned int karnov_readmem(unsigned int address)
{
	if (address < 0x600)
	{	
		return workram[address];
	}
	
	if (address == 0x800)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static unsigned int karnov_readop(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static void karnov_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x600)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x1000 && address <= 0x1001)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x1800 && address <= 0x1801)
	{
		YM3526Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x8000)
	{
		return;
	}

//	printf("unk write %x at %x\n", data, address);
}

static unsigned int sidep_readmem(unsigned int address)
{
	if (address < 0x1000)
	{	
		return workram[address];
	}
	
	if (address == 0x3000)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static unsigned int sidep_readop(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static void sidep_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x1000)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x1000 && address <= 0x1001)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x2000 && address <= 0x2001)
	{
		YM3526Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x8000)
	{
		return;
	}

//	printf("unk write %x at %x\n", data, address);
}

static unsigned int dec0_readmem(unsigned int address)
{
	if (address < 0x600)
	{	
		return workram[address];
	}
	
	if (address == 0x3000)
	{
//		printf("Read %x from latch\n", cmd_latch);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		return cmd_latch;
	}

	if (address == 0x3800) return OKIM6295_status_0_r(0);

	if (address >= 0x4000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static unsigned int dec0_readop(unsigned int address)
{
	if (address >= 0x4000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static void dec0_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x600)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x800 && address <= 0x801)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x1000 && address <= 0x1001)
	{
		YM3812Write(0, address&0x1, data);
		return;
	}

	if (address == 0x3800)
	{
		OKIM6295_data_0_w(0, data);
		return;
	}

	if (address >= 0x8000)
	{
		return;
	}

//	printf("unk write %x at %x\n", data, address);
}

static unsigned int dec1_readmem(unsigned int address)
{
	if (address < 0x600)
	{	
		return workram[address];
	}
	
	if (address == 0x3000)
	{
//		printf("Read %x from latch\n", cmd_latch);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static unsigned int dec1_readop(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unk read at %x\n", address);
	return 0;
}

static void dec1_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x600)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x800 && address <= 0x801)
	{
		YM2203Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x1000 && address <= 0x1001)
	{
		YM3526Write(0, address&0x1, data);
		return;
	}

	if (address >= 0x8000)
	{
		return;
	}

//	printf("unk write %x at %x\n", data, address);
}

/* YM3526 IRQ handler */
static void OPLIRQ(int irq)
{
//	printf("OPL IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);
}

static void SIDEP_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
