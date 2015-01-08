/* Burgertime board */

#include "m1snd.h"

#define M6502_CLOCK (500000)
#define AY_CLOCK (1500000)

#define VBL_RATE (1.0/960.0)

#define TIMER_LINE	IRQ_LINE_NMI
#define CMD_LINE  	M6502_IRQ_LINE

static void BT_Init(long srate);
static void BT_SendCmd(int cmda, int cmdb);

static unsigned int bt_readmem(unsigned int address);
static void bt_writemem(unsigned int address, unsigned int data);

static int cmd_latch, irq_enable = 0;

static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? (hand tuned) */
	{ MIXER(23,MIXER_PAN_CENTER), MIXER(23,MIXER_PAN_CENTER) },	/* dotron clips with anything higher */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static M16502T btrw = 
{
	bt_readmem,
	bt_readmem,
	bt_writemem,
};

static void Write_AY8910(unsigned short addr, unsigned char value)
{
//	printf("Write %02x to 8910 at %04x\n", value, addr);

	if ((addr&0xf000) == 0x2000)
	{
		AY8910_write_port_0_w(0, value);
	}

	if ((addr&0xf000) == 0x4000)
	{
		AY8910_control_port_0_w(0, value);
	}

	if ((addr&0xf000) == 0x6000)
	{
		AY8910_write_port_1_w(0, value);
	}

	if ((addr&0xf000) == 0x8000)
	{
		AY8910_control_port_1_w(0, value);
	}
}

static unsigned int bt_readmem(unsigned int address)
{
	if (address <= 0x3ff)
	{	
		return workram[address];
	}
	
	if ((address & 0xf000) == 0xa000)
	{
//		printf("read latch %x\n", cmd_latch);
		m6502_set_irq_line(CMD_LINE, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0xf000)
	{
		return prgrom[address-0xf000];
	}

	return 0;
}

static void bt_writemem(unsigned int address, unsigned int data)
{
	if (address <= 0x3ff)
	{	
		workram[address] = data;
		return;
	}

	if (address >= 0x2000 && address <= 0x8fff)
	{
		Write_AY8910(address, data);
	}

	if ((address & 0xf000) == 0xc000)
	{
		//printf("irq enabled at %x\n", irq_enable);
		irq_enable = 1;
	}
}
 
static void vbl_timer(int refcon);

static void vbl_timer2(int refcon)
{
	m6502_set_irq_line(TIMER_LINE, CLEAR_LINE);
	timer_set(VBL_RATE/2, 0, vbl_timer);
}

static void vbl_timer(int refcon)
{
	if (irq_enable)
	{
		m6502_set_irq_line(TIMER_LINE, ASSERT_LINE);
	}

	timer_set(VBL_RATE/2, 0, vbl_timer2);
}

static void BT_Init(long srate)
{
	timer_set(VBL_RATE, 0, vbl_timer);
}

static void BT_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	m6502_set_irq_line(CMD_LINE, ASSERT_LINE);
}

M1_BOARD_START( btime )
	MDRV_NAME("BurgerTime")
	MDRV_HWDESC("6502, AY-3-8910(x2)")
	MDRV_INIT( BT_Init )
	MDRV_SEND( BT_SendCmd )
	MDRV_DELAYS( 1000, 15 )

	MDRV_CPU_ADD(M6502, M6502_CLOCK)
	MDRV_CPUMEMHAND(&btrw)
	
	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
M1_BOARD_END
