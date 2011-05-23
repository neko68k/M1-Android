// Super Shanghai (Z80 + YM2203 + MSM6295)

#include "m1snd.h"

static void SSh_SendCmd(int cmda, int cmdb);

static unsigned int ssh_read(unsigned int address);
static void ssh_write(unsigned int address, unsigned int data);
static unsigned int ssh_readport(unsigned int address);
static void ssh_writeport(unsigned int address, unsigned int data);

static int cmd_latch;

M1Z80T ssh_rw =
{
	ssh_read,
	ssh_read,
	ssh_write,
	ssh_readport,
	ssh_writeport
};

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ RGN_SAMP1 },      /* memory region */
	{ 50 }
};

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,
	16000000/4,
	{ YM2203_VOL(60,60) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

M1_BOARD_START( sshanghai )
	MDRV_NAME("Super Shanghai")
	MDRV_HWDESC("Z80, YM2203, MSM-6295")
	MDRV_SEND( SSh_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPUMEMHAND(&ssh_rw)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static unsigned int ssh_read(unsigned int address)
{
	if (address <= 0x7fff) return prgrom[address];
	if (address == 0xc000) return YM2203_status_port_0_r(0);
	if (address >= 0xf800 && address <= 0xffff) return workram[address];
//	if (address == 0xf80a) return OKIM6295_status_0_r(0);

	printf("Unk read %x\n", address);
 	return 0;
}

static void ssh_write(unsigned int address, unsigned int data)
{
	if (address >= 0xf800 && address <= 0xffff)
	{
		if (address < 0xff00)
			printf("read work at %x\n", address);
		workram[address] = data;
		return;
	}
	if (address == 0xc000)
	{
		YM2203_control_port_0_w(0, data);
		return;
	}
	if (address == 0xc001)
	{
		YM2203_write_port_0_w(0, data);
		return;
	}

	printf("Unk write %x to %x\n", data, address);
}

static unsigned int ssh_readport(unsigned int address)
{
	printf("Unk port read %x\n", address);
	return 0;
}

static void ssh_writeport(unsigned int address, unsigned int data)
{
	printf("Unk port write %x to %x\n", data, address);
}

static void SSh_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	workram[0xf809] = 0x10;
	workram[0xf808] = cmda;
 //	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
