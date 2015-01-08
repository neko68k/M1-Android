/* DECO Lemmings.  This is pretty unlike their other sound boards - uses a 6809
   instead of the usual 6502 or Hu6280 */

#include "m1snd.h"

#define M6809_CLOCK (32220000/8)
#define YM_CLOCK (32220000/9)
#define MSM_CLOCK (7759)

static void L_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);

static unsigned int L_Read(unsigned int address);
static void L_Write(unsigned int address, unsigned int data);

static int cmd_latch;
static int ym2151_last_adr;

static M16809T lemmingsrwmem =
{
	L_Read,
	L_Write,
};

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ RGN_SAMP1 },	/* memory region */
	{ 50 }
};

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9,
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static void YM2151_IRQ(int irq)
{
	if (irq)
		cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
}

static unsigned int L_Read(unsigned int address)
{
	if (address < 0x800)
	{
		return workram[address];
	}

	if (address == 0x801)
	{
		return YM2151ReadStatus(0);
	}

	if (address == 0x1000)
	{
		return OKIM6295_status_0_r(0);
	}

	if (address == 0x1800)
	{
		cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void L_Write(unsigned int address, unsigned int data)
{					
	if (address < 0x800)
	{
		workram[address] = data;
		return;
	}

	if (address == 0x800)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x801)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address == 0x1000)
	{	
		OKIM6295_data_0_w(0, data);
	}

	if (address == 0x1800)
	{
		return;	// ack to main cpu
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void L_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, M6809_FIRQ_LINE, HOLD_LINE);
}

M1_BOARD_START( lemmings )
	MDRV_NAME("Lemmings")
	MDRV_HWDESC("6809, YM2151, MSM-6295")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( L_SendCmd )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&lemmingsrwmem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

