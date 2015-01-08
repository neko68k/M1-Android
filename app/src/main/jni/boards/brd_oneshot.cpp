// "One Shot One Kill" cheezy lightgun game

#include "m1snd.h"

static void OsoSendCmd(int cmda, int cmdb);

static unsigned int oso_read(unsigned int address);
static void oso_write(unsigned int address, unsigned int data);
static unsigned int oso_readport(unsigned int address);
static void oso_writeport(unsigned int address, unsigned int data);

static int cmd_latch;

M1Z80T oso_rw =
{
	oso_read,
	oso_read,
	oso_write,
	oso_readport,
	oso_writeport
};

static void irq_handler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
	1,
	5000000,
	{ 70 },
	{ irq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 8000, 8000 },	/* ? */
	{ RGN_SAMP1, RGN_SAMP2 },
	{ 70, 70 }
};

M1_BOARD_START( oneshot )
	MDRV_NAME("One Shot One Kill")
	MDRV_HWDESC("Z80, YM3812, MSM-6295(x2?)")
	MDRV_SEND( OsoSendCmd )

	MDRV_CPU_ADD(Z80C, 5000000)
	MDRV_CPUMEMHAND(&oso_rw)
	
	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static unsigned int oso_read(unsigned int address)
{
	if (address <= 0x7fff) return prgrom[address];
	if (address >= 0x8000 && address <= 0x87ff) return workram[address];
	if (address == 0xe000) return YM3812_status_port_0_r(0);
#if 1
	if (address == 0xe010) //return OKIM6295_status_0_r(0);
	{
//		printf("read latch at %x\n", cmd_latch);
		return cmd_latch;
	}
#endif
	printf("Unknown read at %x (PC=%x)\n", address, z80_get_reg(REG_PC));
 	return 0;
}

static void oso_write(unsigned int address, unsigned int data)
{
	if (address >= 0x8000 && address <= 0x87ff)
	{
		workram[address] = data;
		return;
	}

	if (address == 0xe000)
	{
//		OKIM6295_data_0_w(0, data);
 		YM3812_control_port_0_w(0, data);
		return;
	}
	if (address == 0xe001)
	{
//		OKIM6295_data_1_w(0, data);
   		YM3812_write_port_0_w(0, data);
		return;
	}
/*	if (address == 0xe010)
	{
		OKIM6295_data_0_w(0, data);
		return;
	}*/

	printf("Unknown write %x at %x\n", data, address);
}

static unsigned int oso_readport(unsigned int port)
{
	printf("Unknown port read at %x\n", port);
	return 0;
}

static void oso_writeport(unsigned int port, unsigned int data)
{
	printf("Unknown port write %x at %x\n", data, port);
}

static void OsoSendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}
