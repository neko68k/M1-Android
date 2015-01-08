// Jaleco Mega System 1 (68000 + YM2151 + 2xMSM-6295)
// also Mega System 1 "Z" varient: Z80 + YM2203

#include "m1snd.h"

static void MS1_SendCmd(int cmda, int cmdb);
static void MS1Z_SendCmd(int cmda, int cmdb);

static unsigned int ms1_read_memory_8(unsigned int address);
static unsigned int ms1_read_memory_16(unsigned int address);
static unsigned int ms1_read_memory_32(unsigned int address);
static void ms1_write_memory_8(unsigned int address, unsigned int data);
static void ms1_write_memory_16(unsigned int address, unsigned int data);
static void ms1_write_memory_32(unsigned int address, unsigned int data);

static void YM2151_IRQ(int irq);
static void YM2203_IRQ(int irq);

static int ym2151_last_adr, cmd_latch;

static struct YM2151interface ym2151_interface =
{
	1,
	7000000/2,
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 4000000/132, 4000000/132 },	/* seems appropriate */
	{ RGN_SAMP1, RGN_SAMP2 },
	{ MIXER(25,MIXER_PAN_CENTER), MIXER(25,MIXER_PAN_CENTER) }
};

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* ??? */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM2203_IRQ }
};

static M168KT ms1_readwritemem =
{
	ms1_read_memory_8,
	ms1_read_memory_16,
	ms1_read_memory_32,
	ms1_write_memory_8,
	ms1_write_memory_16,
	ms1_write_memory_32,
};

static void Dec32_Update(long dsppos, long dspframes)
{
	if (Machine->refcon == 1)
	{
		ym2151_hack = 0.8;
	}

	StreamSys_Update(dsppos, dspframes);
}

M1_BOARD_START( megasys1 )
	MDRV_NAME("Jaleco Mega System 1 (68k version)")
	MDRV_HWDESC("68000, YM2151, MSM-6295(x2)")
	MDRV_UPDATE( Dec32_Update )
	MDRV_SEND( MS1_SendCmd )

	MDRV_CPU_ADD(MC68000, 7000000)
	MDRV_CPUMEMHAND(&ms1_readwritemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static MEMORY_READ_START( sound_readmem_z80 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_z80 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_NOP }, /* ?? */
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

M1_BOARD_START( megasys1z80 )
	MDRV_NAME("Jaleco Mega System 1 (Z80 version)")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_SEND( MS1Z_SendCmd )

	MDRV_CPU_ADD(Z80C, 3000000)
	MDRV_CPU_MEMORY(sound_readmem_z80,sound_writemem_z80)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static void YM2203_IRQ(int irq)
{
//	printf("YM2203 IRQ! irq=%d\n", irq);

	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void YM2151_IRQ(int irq)
{
	if (irq)
	{
		m68k_set_irq(M68K_IRQ_4);	// IRQ4 on timer
	}
}

// 68k stuff
static unsigned int ms1_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x20000)
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

	if (address >= 0xa0000 && address <= 0xc0001)
	{
		return 0;	// MSM6295 status hack as per MAME
	}

	if (address >= 0xe0000 && address <= 0xfffff)
	{
		return workram[address-0xe0000];
	}

//	printf("Unknown read 8 at %x\n", address);

	return 0;
}

static unsigned int ms1_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x20000)
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

	if (address == 0x80002)
	{
		return YM2151ReadStatus(0) | (YM2151ReadStatus(0)<<8);
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

static unsigned int ms1_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x20000)
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

static void ms1_write_memory_8(unsigned int address, unsigned int data)
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

//	printf("Unknown write 8 %x to %x\n", data, address);
}

static void ms1_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

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

//	printf("Unknown write 16 %x to %x\n", data, address);
}

static void ms1_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0xe0000 && address <= 0xfffff)
	{
		address -= 0xe0000;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}

//	printf("Unknown write 32 %x to %x\n", data, address);
}

static void MS1_SendCmd(int cmda, int cmdb)
{
	if (cmda < 256)
	{
		cmd_latch = cmda;	
	}
	else
	{
		cmd_latch = (cmda-256)<<8;
	}

	
}

static void MS1Z_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
