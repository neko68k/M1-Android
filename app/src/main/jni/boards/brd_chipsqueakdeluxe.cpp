// Bally/Midway "Chip Squeak Deluxe" (Spy Hunter)

#include "m1snd.h"

static void CSD_Init(long srate);
static void CSD_SendCmd(int cmda, int cmdb);

static unsigned int csd_read_memory_8(unsigned int address);
static unsigned int csd_read_memory_16(unsigned int address);
static unsigned int csd_read_memory_32(unsigned int address);
static void csd_write_memory_8(unsigned int address, unsigned int data);
static void csd_write_memory_16(unsigned int address, unsigned int data);
static void csd_write_memory_32(unsigned int address, unsigned int data);

static INT16 dacval;

static M168KT csd_readwritemem =
{
	csd_read_memory_8,
	csd_read_memory_16,
	csd_read_memory_32,
	csd_write_memory_8,
	csd_write_memory_16,
	csd_write_memory_32,
};

struct DACinterface mcr_dac_interface =
{
	1,
	{ 100 }
};

static unsigned int csd_read_memory_8(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x8000)
	{
		return prgrom[address];
	}

	if (address >= 0x18000 && address <= 0x18007)
	{
		return pia_0_r((address&0xf)>>1);
	}

	if (address >= 0x1c000 && address <= 0x1cfff)
	{
		return workram[address];
	}

	printf("Unknown read 8 at %x PC=%x\n", address, m68k_get_reg(NULL, M68K_REG_PC));

	return 0;
}

static unsigned int csd_read_memory_16(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x8000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if ((address >= 0x1c000) && (address <= 0x1cfff))
	{
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	printf("Unknown read 16 at %x PC=%x\n", address, m68k_get_reg(NULL, M68K_REG_PC));
	return 0;
}

static unsigned int csd_read_memory_32(unsigned int address)
{
	address &= 0xffffff;

	if (address < 0x8000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if ((address >= 0x1c000) && (address <= 0x1cfff))
	{
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

	printf("Unknown read 32 at %x PC=%x\n", address, m68k_get_reg(NULL, M68K_REG_PC));
	return 0;
}

static void csd_write_memory_8(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x18000 && address < 0x18008)
	{
		pia_0_w((address&0xf)>>1, data);
		return;
	}

	if (address >= 0x1c000 && address <= 0x1cfff)
	{
		workram[address] = data;
		return;
	}

	printf("Unknown write 8 %x to %x PC=%x\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void csd_write_memory_16(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x18000 && address < 0x18008)
	{
		pia_0_w((address&0xf)>>1, data>>8);
		return;
	}

	if (address >= 0x1c000 && address <= 0x1cfff)
	{
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}

	printf("Unknown write 16 %x to %x PC=%x\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

static void csd_write_memory_32(unsigned int address, unsigned int data)
{
	address &= 0xffffff;

	if (address >= 0x1c000 && address <= 0x1cfff)
	{
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}

	printf("Unknown write 32 %x to %x PC=%x\n", data, address, m68k_get_reg(NULL, M68K_REG_PC));
}

/********* internal interfaces ***********/
static WRITE_HANDLER( csdeluxe_porta_w )
{
	dacval = (dacval & ~0x3fc) | (data << 2);
	DAC_signed_data_16_w(0, dacval << 6);
}

static WRITE_HANDLER( csdeluxe_portb_w )
{
	dacval = (dacval & ~0x003) | (data >> 6);
	DAC_signed_data_16_w(0, dacval << 6);
}

static void csdeluxe_irq(int state)
{
	if (state)
	{
		printf("IRQ up\n");
		m68k_set_irq(M68K_IRQ_4);
		m68k_set_irq(M68K_IRQ_NONE);
	}
	else
	{
		printf("IRQ down\n");
		m68k_set_irq(M68K_IRQ_NONE);
	}
}

static void csdeluxe_delayed_data_w(int param)
{
//	printf("write %x\n", param);
	pia_0_portb_w(0, param & 0x0f);	// set data from low nibble
	pia_0_ca1_w(0, ~param & 0x10);
}

/********* external interfaces ***********/
WRITE_HANDLER( csdeluxe_data_w )
{
	timer_set(TIME_NOW, data, csdeluxe_delayed_data_w);
}

struct pia6821_interface csdeluxe_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ csdeluxe_porta_w, csdeluxe_portb_w, 0, 0,
	/*irqs   : A/B             */ csdeluxe_irq, csdeluxe_irq
};

#if 0
static int sptr = 0;

static void sh_timer(int refcon)
{
	static int seq[6] = { 0x30, 0xb0, 0xf0, 0xb0, 0x30, 0x70 };

	csdeluxe_delayed_data_w(seq[sptr]);
	sptr++;
	if (sptr >= 6) sptr = 0;

	timer_set(1.0/60.0, 0, sh_timer);
}
#endif

static void CSD_Init(long srate)
{
	pia_config(0, PIA_ALTERNATE_ORDERING, &csdeluxe_pia_intf);

//	timer_set(1.0/60.0, 0, sh_timer);

//	m1snd_addToCmdQueue(0x72);
//	m1snd_addToCmdQueue(0x52);
//	m1snd_addToCmdQueue(0x72);
//	m1snd_addToCmdQueue(0x72);
//	m1snd_addToCmdQueue(0x62);
//	m1snd_addToCmdQueue(0x72);
}

static void CSD_SendCmd(int cmda, int cmdb)
{
	csdeluxe_delayed_data_w((cmda&0xf)|0x10);
	csdeluxe_delayed_data_w(cmda&0xf);
	csdeluxe_delayed_data_w((cmda&0xf)|0x10);
//	csdeluxe_delayed_data_w(cmda);
}

#if 0
M1_ROMS_START( csd )
	{
		"Spy Hunter",
		"1983",
		"",
		"spyhunt",
		MFG_WILLIAMS,
		BOARD_CSD,
		0x72,
		0,
		ROM_START
		     	ROM_REGION( 0x40000, RGN_CPU1, 0 )
			ROM_LOAD16_BYTE( "csd_u7a.u7",   0x00000, 0x2000, CRC(6e689fe7) SHA1(38ad2e9f12b9d389fb2568ebcb32c8bd1ac6879e) )
			ROM_LOAD16_BYTE( "csd_u17b.u17", 0x00001, 0x2000, CRC(0d9ddce6) SHA1(d955c0e67fc78b517cc229601ab4023cc5a644c2) )
			ROM_LOAD16_BYTE( "csd_u8c.u8",   0x04000, 0x2000, CRC(35563cd0) SHA1(5708d374dd56758194c95118f096ea51bf12bf64) )
			ROM_LOAD16_BYTE( "csd_u18d.u18", 0x04001, 0x2000, CRC(63d3f5b1) SHA1(5864a7e9b6bc3d2df6891d40965a7a0efbba6837) )
		ROM_END
		0, 255
	},
M1_ROMS_END
#endif

M1_BOARD_START( chipsqueak )
	MDRV_NAME("Chip Squeak Deluxe")
	MDRV_HWDESC("68000, DAC")
	MDRV_INIT( CSD_Init )
	MDRV_SEND( CSD_SendCmd )

	MDRV_CPU_ADD(MC68000, 15000000/2)
	MDRV_CPUMEMHAND(&csd_readwritemem)

	MDRV_SOUND_ADD(DAC, &mcr_dac_interface)
M1_BOARD_END


