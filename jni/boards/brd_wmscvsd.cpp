/* Williams CVSD boards */

#include "m1snd.h"

#define M6809_CLOCK (2000000)
#define YM_CLOCK (3579580)

static void NARC_Init(long srate);
static void NARC_SendCmd(int cmda, int cmdb);
static void CVSD_Init(long srate);
static void PCVSD_Init(long srate);
static void CVSD_SendCmd(int cmda, int cmdb);
static void PCVSD_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);
static void YM2151_IRQ2(int irq);

static unsigned int CVSD_Read(unsigned int address);
static void CVSD_Write(unsigned int address, unsigned int data);
static unsigned int PCVSD_Read(unsigned int address);
static void PCVSD_Write(unsigned int address, unsigned int data);
static unsigned int NARC_Read(unsigned int address);
static void NARC_Write(unsigned int address, unsigned int data);
static unsigned int NARC2_Read(unsigned int address);
static void NARC2_Write(unsigned int address, unsigned int data);
static WRITE_HANDLER( williams_dac_data_w );
static void williams_cvsd_irqa(int state);
static void williams_cvsd_irqb(int state);
static int bankofs, dac_enable, cmd_latch, bankofs2, cmd_latch2;

/* PIA structure */
static struct pia6821_interface williams_cvsd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ williams_dac_data_w, 0, 0, 0,
	/*irqs   : A/B             */ williams_cvsd_irqa, williams_cvsd_irqb
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(10,MIXER_PAN_CENTER,10,MIXER_PAN_CENTER) },
	{ YM2151_IRQ }
};

static struct YM2151interface narc_ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(10,MIXER_PAN_CENTER,10,MIXER_PAN_CENTER) },
	{ YM2151_IRQ2 }
};

static struct hc55516_interface williams_cvsd_interface =
{
	1,			/* 1 chip */
	{ 80 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

struct DACinterface narc_dac_interface =
{
	2,
	{ 20, 20 }
};

static M16809T cvsdrwmem =
{
	CVSD_Read,
	CVSD_Write,
};

static M16809T pcvsdrwmem =
{
	PCVSD_Read,
	PCVSD_Write,
};

static M16809T narcrwmem =
{
	NARC_Read,
	NARC_Write,
};

static M16809T narcsubrwmem =
{
	NARC2_Read,
	NARC2_Write,
};

M1_BOARD_START( cvsd )
	MDRV_NAME("Williams Y-Unit CVSD")
	MDRV_HWDESC("6809, YM2151, HC55516, DAC")
	MDRV_INIT( CVSD_Init )
	MDRV_SEND( CVSD_SendCmd )
	MDRV_DELAYS( 1000, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&cvsdrwmem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
	MDRV_SOUND_ADD(HC55516, &williams_cvsd_interface)
M1_BOARD_END

M1_BOARD_START( narc )
	MDRV_NAME("NARC")
	MDRV_HWDESC("6809(x2), YM2151, HC55516, DAC(x2)")
	MDRV_INIT( NARC_Init )
	MDRV_SEND( NARC_SendCmd )
	MDRV_DELAYS( 1000, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&narcrwmem)
	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&narcsubrwmem)

	MDRV_SOUND_ADD(YM2151, &narc_ym2151_interface)
	MDRV_SOUND_ADD(DAC, &narc_dac_interface)
	MDRV_SOUND_ADD(HC55516, &williams_cvsd_interface)
M1_BOARD_END

static int ym2151_last_adr;

static void YM2151_IRQ2(int irq)
{
//	printf("YM IRQ: %d\n", irq);
	cpu_set_irq_line(0, M6809_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static void YM2151_IRQ(int irq)
{
	pia_set_input_ca1(0, !irq);
}

static void williams_cvsd_irqa(int state)
{
//	printf("firq: %d\n", state);
	cpu_set_irq_line(0, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void williams_cvsd_irqb(int state)
{
//	printf("nmi: %d\n", state);
	cpu_set_irq_line(0, IRQ_LINE_NMI, state ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( williams_dac_data_w )
{
	if (!dac_enable) return;
	DAC_data_w(0, data);
}

static unsigned int CVSD_Read(unsigned int address)
{
	if (address < 0x2000)
	{
		return workram[address];
	}

	if (address >= 0x2000 && address <= 0x2001)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x4000 && address <= 0x4003)
	{
		return pia_read(0, address&0x3);
	}

	if (address >= 0x8000)
	{
		return prgrom[address-0x8000 + bankofs];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void CVSD_Write(unsigned int address, unsigned int data)
{
	if (address < 0x2000)
	{
		workram[address] = data;
		return;
	}

	if (address == 0x2000)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x2001)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x4000 && address <= 0x4003)
	{	
		pia_write(0, address&0x3, data);
		return;
	}

	if (address == 0x6000)
	{	
		hc55516_0_digit_clock_clear_w(0, data);
		return;
	}

	if (address == 0x6800)
	{	
		hc55516_0_clock_set_w(0, data);
		return;
	}

	if (address == 0x7800)
	{
		int bank = data & 3;
		int quarter = (data >> 2) & 3;

		if (Machine->refcon == 1)
		{
			bankofs = 0x10000*(data & 0x03) + 0x8000*((data & 0x04)>>2);
		}
		else
		{
			if (bank == 3) bank = 0;

			bankofs = 0x10000 + (bank * 0x20000) + (quarter * 0x8000);
		}

//		printf("bankswitch: %x, %x\n", data, bankofs);
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

// pinball version of CVSD board - very similar to the NARC master board

static unsigned int PCVSD_Read(unsigned int address)
{
	if (address <= 0x1fff)
	{
		return workram[address];
	}

	if (address >= 0x2400 && address <= 0x2401)
	{
		return YM2151ReadStatus(0);
	}

	if (address == 0x3000)
	{
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		return prgrom[address - 0x4000 + bankofs];
	}

	if (address >= 0xc000)
	{
//		printf("PCVSD fixed read @ %x = %x\n", (int)address, (int)(address - 0xc000 + (rom_getregionsize(RGN_CPU1)-0x4000)));
		return prgrom[address - 0xc000 + 0x7c000];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void PCVSD_Write(unsigned int address, unsigned int data)
{					
	if (address <= 0x1fff)
	{
		workram[address] = data;
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
		if (!dac_enable) return;
		DAC_data_w(0, data);
	}

	if (address == 0x3400)
	{	
		hc55516_0_digit_clock_clear_w(0, data);
		return;
	}

	if (address == 0x2c00)
	{	
		hc55516_0_clock_set_w(0, data);
		return;
	}

	if (address == 0x3800)
	{
		return;	// volume
	}

	if (address == 0x3c00)
	{
		return; // latch to main
	}
	
	if (address == 0x2000)
	{
		int bank = data & 0xf;

		switch ((~data) & 0xe0) 
		{
		case 0x80: /* U18 */
			bank |= 0x00; 
			break;
		case 0x40: /* U15 */
			bank |= 0x10; break;
		case 0x20: /* U14 */
			bank |= 0x20; break;
		default:
			break;
		}

		bankofs = bank<<15;

//		printf("bankswitch: %x, %x %x\n", data, bank, bankofs);
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static unsigned int NARC_Read(unsigned int address)
{
	if (address < 0x2000)
	{
		return workram[address];
	}

	if (address >= 0x2000 && address <= 0x2001)
	{
		return YM2151ReadStatus(0);
	}

	if (address == 0x3000) return 0;	// ??

	if (address == 0x3400)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		return prgrom[address-0x4000 + bankofs];
	}

	if (address >= 0xc000)
	{
		return prgrom[address - 0xc000 + 0x2c000];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void NARC_Write(unsigned int address, unsigned int data)
{					
	if (address < 0x2000)
	{
		workram[address] = data;
		return;
	}

	if (address == 0x2000)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x2001)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address == 0x2800) return;	// talkback

	if (address == 0x2c00)
	{
		cmd_latch2 = data;
//		printf("write %x to slave\n", data);
		cpu_set_irq_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
		return;
	}

	if (address == 0x3000)
	{
		williams_dac_data_w(0, data);
		return;
	}

	if (address == 0x3800)
	{
		int bank = data & 3;
		if (!(data & 4)) bank = 0;

		bankofs = 0x10000 + (bank * 0x8000);

//		printf("M bankswitch: %x, %x\n", data, bankofs);
		return;
	}

	if (address == 0x3c00) return;	// related to sending commands to the slave

	if (address >= 0x4000) return;

//	printf("Unmapped write %x to %x\n", data, address);
}

static unsigned int NARC2_Read(unsigned int address)
{
	if (address < 0x2000)
	{
		return workram[address+0x20000];
	}

	if (address == 0x3000) return 0;	// ??

	if (address == 0x3400)
	{
//		printf("sub: read %x from latch\n", cmd_latch2);
		cpu_set_irq_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
		return cmd_latch2;
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		return prgrom2[address-0x4000 + bankofs2];
	}

	if (address >= 0xc000)
	{
		return prgrom2[address - 0xc000 + 0x4c000];
	}

//	printf("S: Unmapped read at %x\n", address);
	return 0;
}

static void NARC2_Write(unsigned int address, unsigned int data)
{					
	if (address < 0x2000)
	{
		workram[address+0x20000] = data;
		return;
	}

	if (address == 0x2000)
	{
		hc55516_0_clock_set_w(0, data);
		return;
	}

	if (address == 0x2400)
	{
		hc55516_0_digit_clock_clear_w(0, data);
		return;
	}

	if (address == 0x3000)
	{
		if (!dac_enable) return;
		DAC_data_w(1, data);
		return;
	}

	if (address == 0x3800)
	{
		int bank = data & 7;
		bankofs2 = 0x10000 + (bank * 0x8000);

//		printf("S bankswitch: %x, %x\n", data, bankofs);
		return;
	}

//	printf("S: Unmapped write %x to %x\n", data, address);
}

static void CVSD_Init(long srate)
{
	dac_enable = 0;

	pia_config(0, PIA_STANDARD_ORDERING, &williams_cvsd_pia_intf);

	pia_set_input_ca1(0, 1);

	CVSD_Write(0x7800, 0);	// default to bank 0

	m1snd_addToCmdQueue(0);
}

static void PCVSD_Init(long srate)
{
	dac_enable = 1;

	PCVSD_Write(0x2000, 0x7f);	// default to bank 0
	PCVSD_Write(0x2000, 0);	// default to bank 0

	m1snd_addToCmdQueue(0);
}

static void NARC_Init(long srate)
{
	dac_enable = 0;

	NARC_Write(0x3800, 0);	// default to bank 0

	m1snd_addToCmdQueue(0);
//	m1snd_addToCmdQueue(0xff);

	cmd_latch = 0xff;
}

static void write_cvsd(int cmda)
{
	pia_set_input_b(0, cmda & 0xff);
	pia_set_input_cb1(0, cmda & 0x100);
	pia_set_input_cb2(0, cmda & 0x200);
}

static void write_narc(int cmda)
{
	cmd_latch = cmda;
	if (!(cmda & 0x100))
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	}

	if (!(cmda & 0x200))
	{
		cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	}
}

static void NARC_SendCmd(int cmda, int cmdb)
{
	dac_enable = 1;
	write_narc(0xfd00 | cmda);
	write_narc(0xff00 | cmda);
}

static void CVSD_SendCmd(int cmda, int cmdb)
{
	dac_enable = 1;
	write_cvsd(cmda);
	write_cvsd(cmda | 0x100);
}

static void PCVSD_SendCmd(int cmda, int cmdb)
{
	dac_enable = 1;
	cmd_latch = cmda;
	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
}

M1_BOARD_START( pincvsd )
	MDRV_NAME("Williams Pinball CVSD")
	MDRV_HWDESC("6809, YM2151, HC55516, DAC")
	MDRV_INIT( PCVSD_Init )
	MDRV_SEND( PCVSD_SendCmd )
	MDRV_DELAYS( 1000, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&pcvsdrwmem)

	MDRV_SOUND_ADD(YM2151, &narc_ym2151_interface)
	MDRV_SOUND_ADD(DAC, &narc_dac_interface)
	MDRV_SOUND_ADD(HC55516, &williams_cvsd_interface)
M1_BOARD_END

