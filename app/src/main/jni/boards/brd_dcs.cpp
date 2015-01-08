// Midway DCS (Digital Compressed Sound)

#include "m1snd.h"

#define DEBUG_DCS				0
#define SPEEDUP_BOOT				1

extern "C" {
unsigned dasm2100(char *buffer, unsigned pc);
void adsp2104_reset(void *param);
void adsp2105_reset(void *param);
void adsp2104_get_info(UINT32 state, union cpuinfo *info);
void adsp2104_set_info(UINT32 state, union cpuinfo *info);
}

static void DCS_Init(long srate);
static void DCS95_Init(long srate);
static void DCSRAM_Init(long srate);
static void DCS_SendCmd(int cmda, int cmdb);
static void DCS_Shutdown(void);
static unsigned int dcs_read8(unsigned int address);
static unsigned int dcs_read16(unsigned int address);
static unsigned int dcs_read32(unsigned int address);
static void dcs_write8(unsigned int address, unsigned int data);
static void dcs_write16(unsigned int address, unsigned int data);
static void dcs_write32(unsigned int address, unsigned int data);

static unsigned int dcs95_read16(unsigned int address);
static void dcs95_write16(unsigned int address, unsigned int data);

static unsigned int dcsram_read16(unsigned int address);
static void dcsram_write16(unsigned int address, unsigned int data);

static int dcs_custom_start(const struct MachineSound *msound);
static void dcs_dac_update(int num, INT16 *buffer, int length);

static UINT16 dcsram_fixed_sram_2[0x1000];

static union cpuinfo info;

unsigned int cpunum_get_reg(int cpu, int reg) 
{ 
	adsp2104_get_info(CPUINFO_INT_REGISTER + reg, &info); 
	return info.i;
}

#define adsp2105_get_reg(x) cpunum_get_reg(0, x)
#define cpunum_set_reg(cpu, reg, val) { info.i = val; adsp2104_set_info(CPUINFO_INT_REGISTER + reg, &info); }

#define adsp2105_set_irq_line(line, val) { info.i = val; adsp2104_set_info(CPUINFO_INT_INPUT_STATE + line, &info); }

typedef INT32 (*RX_CALLBACK)( int port );
typedef void  (*TX_CALLBACK)( int port, INT32 data );

void adsp2105_set_tx_callback(TX_CALLBACK cb)
{
	info.p = (void *)cb;
	adsp2104_set_info(CPUINFO_PTR_ADSP2100_TX_HANDLER, &info);
}

static struct CustomSound_interface dcs_custom_interface =
{
	dcs_custom_start,0,0
};

M1ADSPT dcs_rw =
{
	dcs_read8,
	dcs_read16,
	dcs_read32,
	dcs_write8,
	dcs_write16,
	dcs_write32,
};

M1ADSPT dcs95_rw =
{
	dcs_read8,
	dcs95_read16,
	dcs_read32,
	dcs_write8,
	dcs95_write16,
	dcs_write32,
};

M1ADSPT dcsram_rw =
{
	dcs_read8,
	dcsram_read16,
	dcs_read32,
	dcs_write8,
	dcsram_write16,
	dcs_write32,
};

M1_BOARD_START( dcs )
	MDRV_NAME( "Midway Digital Compressed Sound (DCS)" )
	MDRV_HWDESC( "ADSP-2105, DAC" )
#if (!SPEEDUP_BOOT)
	MDRV_DELAYS( 5000, 100 )
#else
	MDRV_DELAYS( 250, 100 )
#endif
	MDRV_INIT( DCS_Init )
	MDRV_SEND( DCS_SendCmd )
	MDRV_SHUTDOWN( DCS_Shutdown )

#if SPEEDUP_BOOT
	MDRV_CPU_ADD(ADSP2105, 10000000/2)
#else	// don't slow down the ram/rom test worse
	MDRV_CPU_ADD(ADSP2105, 10000000)
#endif
	MDRV_CPUMEMHAND(&dcs_rw)

	MDRV_SOUND_ADD(CUSTOM, &dcs_custom_interface)
M1_BOARD_END

M1_BOARD_START( dcs95 )
	MDRV_NAME( "Midway DCS95" )
	MDRV_HWDESC( "ADSP-2105, DAC" )
#if (!SPEEDUP_BOOT)
	MDRV_DELAYS( 5000, 50 )
#else
	MDRV_DELAYS( 350, 50 )
#endif
	MDRV_INIT( DCS95_Init )
	MDRV_SEND( DCS_SendCmd )
	MDRV_SHUTDOWN( DCS_Shutdown )

#if SPEEDUP_BOOT
	MDRV_CPU_ADD(ADSP2105, 10000000/2)
#else	// don't slow down the ram/rom test worse
	MDRV_CPU_ADD(ADSP2105, 10000000)
#endif
	MDRV_CPUMEMHAND(&dcs95_rw)

	MDRV_SOUND_ADD(CUSTOM, &dcs_custom_interface)
M1_BOARD_END

#define DCS_BUFFER_SIZE				4096
#define DCS_BUFFER_MASK				(DCS_BUFFER_SIZE - 1)

#define LCTRL_OUTPUT_EMPTY			0x400
#define LCTRL_INPUT_EMPTY			0x800

#define IS_OUTPUT_EMPTY()			(dcs.latch_control & LCTRL_OUTPUT_EMPTY)
#define IS_OUTPUT_FULL()			(!(dcs.latch_control & LCTRL_OUTPUT_EMPTY))
#define SET_OUTPUT_EMPTY()			(dcs.latch_control |= LCTRL_OUTPUT_EMPTY)
#define SET_OUTPUT_FULL()			(dcs.latch_control &= ~LCTRL_OUTPUT_EMPTY)

#define IS_INPUT_EMPTY()			(dcs.latch_control & LCTRL_INPUT_EMPTY)
#define IS_INPUT_FULL()				(!(dcs.latch_control & LCTRL_INPUT_EMPTY))
#define SET_INPUT_EMPTY()			(dcs.latch_control |= LCTRL_INPUT_EMPTY)
#define SET_INPUT_FULL()			(dcs.latch_control &= ~LCTRL_INPUT_EMPTY)

#define DCS	0
#define DCS95	1
#define DCSRAM	2

struct dcs_state
{
	int		stream;
	int	version;

	UINT8 * mem;
	UINT16	size;
	UINT16	incs;
	void  * reg_timer;
	void  * sport_timer;
	int has_sport_timer;
	int		ireg;
	UINT16	ireg_base;
	UINT16	control_regs[32];

	UINT16	rombank, rombank2;
	UINT16	rombank_count;
	UINT16	srambank;
	UINT16	drambank;
	UINT16	drambank_count;
	UINT16  databank;
	UINT8	enabled;

	INT16 *	buffer;
	UINT32	buffer_in;
	UINT32	sample_step;
	UINT32	sample_position;
	INT16	current_sample;

	UINT16	latch_control;
	UINT16	input_data;
	UINT16	output_data;
	UINT16	output_control;

	void	(*notify)(int);
};

static INT8 dcs_cpunum;

static struct dcs_state dcs;

static data16_t *dcs_sram_bank0;
static data16_t *dcs_sram_bank1;
static data16_t *dcs_expanded_rom;

static data8_t dcs_banked_dram[0x8000];

#if DEBUG_DCS
static int transfer_state;
static int transfer_start;
static int transfer_stop;
static int transfer_writes_left;
static UINT16 transfer_sum;
#endif

static READ16_HANDLER( dcs_sdrc_asic_ver_r );

static WRITE16_HANDLER( dcs_rombank_select_w );
static READ16_HANDLER( dcs_rombank_data_r );
static READ16_HANDLER( dcs_rombank95_data_r );
//static WRITE16_HANDLER( dcs_sram_bank_w );
//static READ16_HANDLER( dcs_sram_bank_r );
//static WRITE16_HANDLER( dcs_dram_bank_w );
//static READ16_HANDLER( dcs_dram_bank_r );

static WRITE16_HANDLER( dcs_control_w );

static READ16_HANDLER( latch_status_r );
static READ16_HANDLER( input_latch_r );
static WRITE16_HANDLER( output_latch_w );
static READ16_HANDLER( output_control_r );
static WRITE16_HANDLER( output_control_w );

static void dcs_irq(int state);
static void sport0_irq(int state);
static void sound_tx_callback(int port, INT32 data);

static unsigned int dcs_read8(unsigned int address)
{
	return prgrom[address];
}

static unsigned int dcs_read16(unsigned int address)
{
	int daddress = (address >> 1);

	if (daddress >= 0x2000 && daddress <= 0x2fff)
	{
		return dcs_rombank_data_r((daddress&0xfff), 0);
	}

	if (daddress >= 0x3400 && daddress <= 0x3403)
	{
		return input_latch_r((daddress&0x3), 0);
	}

	return mem_readword((unsigned short *)(prgrom+address));
}

static unsigned int dcs_read32(unsigned int address)
{
	if (address == 0xffffffff)
	{
		return input_latch_r(0, 0)<<8;
	}

	return mem_readlong((unsigned int *)(&prgrom[address+ADSP2100_PGM_OFFSET]));
}

static void dcs_write8(unsigned int address, unsigned int data)
{
  	prgrom[address] = data;
}

static void dcs_write16(unsigned int address, unsigned int data)
{
	int daddress;
	
	daddress = (address >> 1);

	if (daddress == 0x3000)
	{
		dcs_rombank_select_w(0, data, 0);
		return;
	}

	if (daddress >= 0x3400 && daddress <= 0x3403)
	{
		output_latch_w((daddress&0x3), data, 0);
		return;
	}

	if (daddress >= 0x3fe0 && daddress <= 0x3fff)
	{
		dcs_control_w((daddress-0x3fe0), data, 0);
		return;
	}

	mem_writeword((unsigned short *)(prgrom+address), data);
}

static void dcs_write32(unsigned int address, unsigned int data)
{
	mem_writelong((unsigned int *)(&prgrom[address+ADSP2100_PGM_OFFSET]), data);
}

static unsigned int dcs95_read16(unsigned int address)
{
	int daddress = (address >> 1);

	if (daddress <= 0x7ff)
	{
		return dcs_rombank95_data_r((daddress&0x7ff), 0);
	}

	if (daddress >= 0x2000 && daddress <= 0x2fff)
	{
		return mem_readword((unsigned short *)(dcs_banked_dram+address+dcs.drambank));
	}

	if (daddress >= 0x3300 && daddress <= 0x3303)
	{
		return input_latch_r((daddress&0x3), 0);
	}

	return mem_readword((unsigned short *)(prgrom+address));
}

static void dcs95_write16(unsigned int address, unsigned int data)
{
	int daddress;
	
	daddress = (address >> 1);

	if (daddress >= 0x2000 && daddress <= 0x2fff)
	{
		mem_writeword((unsigned short *)(dcs_banked_dram+address+dcs.drambank), data);
		return;
	}

	if (daddress == 0x3000)
	{
		dcs.rombank = data;
		return;
	}

	if (daddress == 0x3100)
	{
		dcs.rombank2 = data;
		return;
	}

	if (daddress == 0x3200)
	{
		mem_writeword((unsigned short *)(prgrom+address), data);
		if (data & 0x8)
		{
			dcs.drambank = 0x2000;
		}
		else
		{
			dcs.drambank = 0;
		}
		return;
	}

	if (daddress >= 0x3300 && daddress <= 0x3303)
	{
		output_latch_w((daddress&0x3), data, 0);
		return;
	}

	if (daddress >= 0x3fe0 && daddress <= 0x3fff)
	{
		dcs_control_w((daddress-0x3fe0), data, 0);
		return;
	}

	mem_writeword((unsigned short *)(prgrom+address), data);
}

static unsigned int dcsram_read16(unsigned int address)
{
	int daddress = (address >> 1);

//	printf("r16: ad %x da %x\n", address, daddress);

	if (daddress <= 0x3ff)	// ROM bank
	{
//		printf("Read ROM bank @ %x\n", ADSP2100_SIZE+daddress+((dcs.databank&0x3ff)*0x400));
		return mem_readword((unsigned short *)(prgrom+ADSP2100_SIZE+daddress+((dcs.databank&0x3ff)*0x400)));
	}
	if (daddress == 0x400)
	{
		return input_latch_r(0, 0);
	}
	if (daddress == 0x402)
	{
		return output_control_r(0, 0);
	}
	if (daddress == 0x403)
	{
		return latch_status_r(0, 0);
	}
	if (daddress == 0x480)
	{
//		printf("Get SRAM bank (PC=%x)\n", adsp2101_get_reg(CPUINFO_INT_PC));
		return dcs.srambank;
  	}
	if (daddress == 0x482)
	{
//		printf("Get DRAM bank (PC=%x)\n", adsp2101_get_reg(CPUINFO_INT_PC));
		return dcs.databank;
	}
	if (daddress == 0x483)
	{
		printf("Get SDRC version (PC=%x)\n", adsp2105_get_reg(CPUINFO_INT_PC));
		return dcs_sdrc_asic_ver_r(0, 0);
	}

	if (daddress >= 0x800 && daddress <= 0x17ff)	// fixed SRAM
	{
		return dcsram_fixed_sram_2[daddress-0x800];
	}

	if (daddress >= 0x1800 && daddress <= 0x27ff)	// SRAM bank
	{
//		printf("Rd SRAM @ %x, bank %x (PC=%x)\n", daddress, dcs.srambank & 0x1000, activecpu_get_pc());
		if (dcs.srambank & 0x1000)
		{
			return mem_readword((unsigned short *)(dcs_banked_dram+address+0x4000));
		}
		else
		{
			return mem_readword((unsigned short *)(dcs_banked_dram+address));
		}
	}

/*	if (daddress >= 0x2800 && daddress <= 0x39ff)
	{
		return dcsram_fixed_sram[daddress-0x2800];
	}*/

	if (address & 0x80000000) return 0;

	return mem_readword((unsigned short *)(prgrom+address));
}

static void dcsram_write16(unsigned int address, unsigned int data)
{
	int daddress;
	
	daddress = (address >> 1);

//	printf("w16: %x to ad %x da %x\n", data, address, daddress);

	if (daddress <= 0x3ff)	// ROM - don't write
	{
		printf("%04x to data @ %x\n", data, address);
		return;
	}

	if (daddress == 0x401)
	{
		output_latch_w(0, data, 0);
		return;
	}
	if (daddress == 0x402)
	{
		output_control_w(0, data, 0);
		return;
	}

	if (daddress == 0x480)
	{
//		printf("%x to SRAM bank (PC=%x)\n", data&0xffff, adsp2105_get_reg(CPUINFO_INT_PC));
		dcs.srambank = data;
		return;
	}

	if (daddress == 0x482)
	{
//		printf("%x to data bank (PC=%x)\n", data&0xffff, adsp2105_get_reg(CPUINFO_INT_PC));
		dcs.databank = data;
		return;
	}

	if (daddress >= 0x800 && daddress <= 0x17ff)	// fixed SRAM
	{
		dcsram_fixed_sram_2[daddress-0x800] = data;
		return;
	}

	if (daddress >= 0x1800 && daddress <= 0x27ff)	// SRAM bank
	{
		if (dcs.srambank & 0x1000)
		{
			mem_writeword((unsigned short *)(dcs_banked_dram+address+0x4000), data);
			return;
		}
		else
		{
			mem_writeword((unsigned short *)(dcs_banked_dram+address), data);
			return;
		}
	}

/*	if (daddress >= 0x2800 && daddress <= 0x39ff)
	{
		dcsram_fixed_sram[daddress-0x2800] = data;
		return;
	}*/

	if (daddress >= 0x3fe0 && daddress <= 0x3fff)
	{
		dcs_control_w((daddress-0x3fe0), data, 0);
		return;
	}

//	printf("%x to prgrom @ %x\n", data, address);
	mem_writeword((unsigned short *)(prgrom+address), data);
}

static void dcs_reset(int bootbank)
{
	unsigned int i;

	/* initialize our state structure and install the transmit callback */
	dcs.mem = 0;
	dcs.size = 0;
	dcs.incs = 0;
	dcs.ireg = 0;

	/* initialize the ADSP control regs */
	for (i = 0; i < sizeof(dcs.control_regs) / sizeof(dcs.control_regs[0]); i++)
		dcs.control_regs[i] = 0;

	/* initialize banking */
	dcs.rombank = 0;
	dcs.srambank = 0;
	dcs.drambank = 0;
	if (dcs_sram_bank0)
	{
//		cpu_setbank(20, memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000);
//		cpu_setbank(21, dcs_sram_bank0);
	}

	/* start with no sound output */
	dcs.enabled = 0;

	/* reset DAC generation */
	dcs.buffer_in = 0;
	dcs.sample_step = 0x10000;
	dcs.sample_position = 0;
	dcs.current_sample = 0;

	/* initialize the ADSP Tx callback */
	adsp2105_set_tx_callback(sound_tx_callback);

	/* clear all interrupts */
	adsp2105_set_irq_line(ADSP2105_IRQ0, CLEAR_LINE);
	adsp2105_set_irq_line(ADSP2105_IRQ1, CLEAR_LINE);
	adsp2105_set_irq_line(ADSP2105_IRQ2, CLEAR_LINE);

	/* initialize the comm bits */
	SET_INPUT_EMPTY();
	SET_OUTPUT_EMPTY();

	/* disable notification by default */
	dcs.notify = NULL;

	/* boot */
	{
#if (!SPEEDUP_BOOT)
		data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE);
#else
	// speed up boot by skipping memory test code
  		data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + ((bootbank & 0x7ff) << 12));
#endif
		data32_t *dst = (data32_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_PGM_OFFSET);
		adsp2105_load_boot_data(src, dst);
	}
	
	/* start the SPORT0 timer if any */
	if (dcs.has_sport_timer)
		dcs.sport_timer = timer_set(TIME_IN_HZ(1000), 0, sport0_irq);
}


void dcs_init(int bootbank)
{
	/* find the DCS CPU */
	dcs_cpunum = 0;
	
	/* reset RAM-based variables */
	dcs_sram_bank0 = dcs_sram_bank1 = NULL;

	/* create the timer */
	dcs.reg_timer = NULL;
	dcs.sport_timer = NULL;
	dcs.has_sport_timer = 0;

	/* reset the system */
	dcs_reset(bootbank);
}


void dcs_ram_init(int bootbank)
{
	UINT8 *romsrc;
	int page, i;

	/* find the DCS CPU */
	dcs_cpunum = 0;

	/* borrow memory for the extra 8k */
	dcs_sram_bank1 = (UINT16 *)(memory_region(REGION_CPU1 + dcs_cpunum) + 0x4000*2);

	/* borrow memory also for the expanded ROM data and expand it */
	romsrc = memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE;
	dcs_expanded_rom = (UINT16 *)(memory_region(REGION_CPU1 + dcs_cpunum) + 0xc000);
	for (page = 0; page < 8; page++)
		for (i = 0; i < 0x400; i++)
			dcs_expanded_rom[0x400 * page + i] = romsrc[BYTE_XOR_LE(0x1000 * page + i)];
	
	/* create the timer */
	dcs.reg_timer = NULL;
	dcs.sport_timer = NULL;
	dcs.has_sport_timer = 1;

	/* reset the system */
	dcs_reset(bootbank);
}



/***************************************************************************
	CUSTOM SOUND INTERFACES
****************************************************************************/

static int dcs_custom_start(const struct MachineSound *msound)
{
	/* allocate a DAC stream */
	dcs.stream = stream_init("DCS DAC", 100, Machine->sample_rate, 0, dcs_dac_update);

	/* allocate memory for our buffer */
	dcs.buffer = (INT16 *)malloc(DCS_BUFFER_SIZE * sizeof(INT16));
	if (!dcs.buffer)
		return 1;

	return 0;
}



/***************************************************************************
	DCS BANK SELECT
****************************************************************************/

static READ16_HANDLER( dcs_sdrc_asic_ver_r )
{
	return 0x5a03;
}



/***************************************************************************
	DCS BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs_rombank_select_w )
{
	dcs.rombank = data & 0x7ff;

	/* bit 11 = sound board led */
#if 0
	set_led_status(2, data & 0x800);
#endif
}


static READ16_HANDLER( dcs_rombank_data_r )
{
	UINT8	*banks = memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE;

	offset += (dcs.rombank & 0x7ff) << 12;
	return banks[offset];
}

static READ16_HANDLER( dcs_rombank95_data_r )
{
	UINT8	*banks = memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE;

	offset += ((dcs.rombank2 & 0x1c)<<18) + ((dcs.rombank2 & 0x1)<<19) + ((dcs.rombank & 0xff)<<11);
//	offset += (dcs.rombank & 0x7fff)<<12;

//	printf("offset: %x\n", offset);
	
	return banks[offset];
}
#if 0
static WRITE16_HANDLER( dcs_sram_bank_w )
{
	COMBINE_DATA(&dcs.srambank);
	cpu_setbank(21, (dcs.srambank & 0x1000) ? dcs_sram_bank1 : dcs_sram_bank0);
}
#endif

#if 0
static READ16_HANDLER( dcs_sram_bank_r )
{
	return dcs.srambank;
}
#endif

#if 0
static WRITE16_HANDLER( dcs_dram_bank_w )
{
	dcs.drambank = data & 0x7ff;
//logerror((char *)"dcs_dram_bank_w(%03X)\n", dcs.drambank);
	cpu_setbank(20, memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000 + dcs.drambank * 0x400*2);
}


static READ16_HANDLER( dcs_dram_bank_r )
{
	return dcs.drambank;
}
#endif


/***************************************************************************
	DCS COMMUNICATIONS
****************************************************************************/

void dcs_set_notify(void (*callback)(int))
{
	dcs.notify = callback;
}


int dcs_control_r(void)
{
	/* this is read by the TMS before issuing a command to check */
	/* if the ADSP has read the last one yet. We give 50 usec to */
	/* the ADSP to read the latch and thus preventing any sound  */
	/* loss */
//	if (IS_INPUT_FULL())
//		cpu_spinuntil_time(TIME_IN_USEC(50));

	return dcs.latch_control;
}


void dcs_reset_w(int state)
{
	logerror((char *)"%08x: DCS reset = %d\n", adsp2105_get_reg(CPUINFO_INT_PC), state);

	/* going high halts the CPU */
	if (state)
	{
		/* just run through the init code again */
		dcs_reset(1);
//		cpu_set_reset_line(dcs_cpunum, ASSERT_LINE);
	}
	
	/* going low resets and reactivates the CPU */
//	else
//		cpu_set_reset_line(dcs_cpunum, CLEAR_LINE);
}


static READ16_HANDLER( latch_status_r )
{
	int result = 0;
	if (IS_INPUT_FULL())
		result |= 0x80;
	if (IS_OUTPUT_EMPTY())
		result |= 0x40;
	return result;
}



/***************************************************************************
	INPUT LATCH (data from host to DCS)
****************************************************************************/

static void delayed_dcs_w(int data)
{
	SET_INPUT_FULL();
	adsp2105_set_irq_line(ADSP2105_IRQ2, ASSERT_LINE);
	dcs.input_data = data;
	
//	timer_set(TIME_IN_USEC(1), 0, NULL);

#if DEBUG_DCS
switch (transfer_state)
{
	case 0:
		if (data == 0x55d0)
		{
			printf("Transfer command\n");
			transfer_state++;
		}
		else if (data == 0x55d1)
		{
			printf("Transfer command alternate\n");
			transfer_state++;
		}
		else
			printf("Command: %04X\n", data);
		break;
	case 1:
		transfer_start = data << 16;
		transfer_state++;
		break;
	case 2:
		transfer_start |= data;
		transfer_state++;
		printf("Start address = %08X\n", transfer_start);
		break;
	case 3:
		transfer_stop = data << 16;
		transfer_state++;
		break;
	case 4:
		transfer_stop |= data;
		transfer_state++;
		printf("Stop address = %08X\n", transfer_stop);
		transfer_writes_left = transfer_stop - transfer_start + 1;
		transfer_sum = 0;
		break;
	case 5:
		transfer_sum += data;
		if (--transfer_writes_left == 0)
		{
			printf("Transfer done, sum = %04X\n", transfer_sum);
			transfer_state = 0;
		}
		break;
}
#endif
}


void dcs_data_w(int data)
{
	timer_set(TIME_NOW, data, delayed_dcs_w);
}


static READ16_HANDLER( input_latch_r )
{
	SET_INPUT_EMPTY();
	adsp2105_set_irq_line(ADSP2105_IRQ2, CLEAR_LINE);
//	printf("Read %x from latch\n", dcs.input_data);
	return dcs.input_data;
}



/***************************************************************************
	OUTPUT LATCH (data from DCS to host)
****************************************************************************/

static void latch_delayed_w(int data)
{
//logerror((char *)"output_data = %04X\n", data);
	if (IS_OUTPUT_EMPTY() && dcs.notify)
		(*dcs.notify)(1);
	SET_OUTPUT_FULL();
	dcs.output_data = data;
}


static WRITE16_HANDLER( output_latch_w )
{
#if DEBUG_DCS
	printf("%04X:Output data = %04X\n", adsp2105_get_reg(CPUINFO_INT_PC), data);
#endif
	timer_set(TIME_NOW, data, latch_delayed_w);
}


int dcs_data_r(void)
{
	/* data is actually only 8 bit (read from d8-d15) */
	if (IS_OUTPUT_FULL() && dcs.notify)
		(*dcs.notify)(0);
	SET_OUTPUT_EMPTY();

	return dcs.output_data;
}



/***************************************************************************
	OUTPUT CONTROL BITS (has 3 additional lines to the host)
****************************************************************************/

static void output_control_delayed_w(int data)
{
	printf("output_control = %04X (PC=%x)\n", data, adsp2105_get_reg(CPUINFO_INT_PC));
	dcs.output_control = data;
}


static WRITE16_HANDLER( output_control_w )
{
	timer_set(TIME_NOW, data, output_control_delayed_w);
}


static READ16_HANDLER( output_control_r )
{
	return dcs.output_control;
}


int dcs_data2_r(void)
{
	return dcs.output_control;
}



/***************************************************************************
	SOUND GENERATION
****************************************************************************/

static void dcs_dac_update(int num, INT16 *buffer, int length)
{
	UINT32 current, step, indx;
	INT16 *source;
	int i;

	/* DAC generation */
	if (dcs.enabled)
	{
		source = dcs.buffer;
		current = dcs.sample_position;
		step = dcs.sample_step;

		/* fill in with samples until we hit the end or run out */
		for (i = 0; i < length; i++)
		{
			indx = current >> 16;
			if (indx >= dcs.buffer_in)
				break;
			current += step;
			*buffer++ = source[indx & DCS_BUFFER_MASK];
		}

if (i < length)
//	logerror((char *)"DCS ran out of input data\n");

		/* fill the rest with the last sample */
		for ( ; i < length; i++)
			*buffer++ = source[(dcs.buffer_in - 1) & DCS_BUFFER_MASK];

		/* mask off extra bits */
		while (current >= (DCS_BUFFER_SIZE << 16))
		{
			current -= DCS_BUFFER_SIZE << 16;
			dcs.buffer_in -= DCS_BUFFER_SIZE;
		}

//logerror((char *)"DCS dac update: bytes in buffer = %d\n", dcs.buffer_in - (current >> 16));

		/* update the final values */
		dcs.sample_position = current;
	}
	else
		memset(buffer, 0, length * sizeof(INT16));
}



/***************************************************************************
	ADSP CONTROL & TRANSMIT CALLBACK
****************************************************************************/

/*
	The ADSP2105 memory map when in boot rom mode is as follows:

	Program Memory:
	0x0000-0x03ff = Internal Program Ram (contents of boot rom gets copied here)
	0x0400-0x07ff = Reserved
	0x0800-0x3fff = External Program Ram

	Data Memory:
	0x0000-0x03ff = External Data - 0 Waitstates
	0x0400-0x07ff = External Data - 1 Waitstates
	0x0800-0x2fff = External Data - 2 Waitstates
	0x3000-0x33ff = External Data - 3 Waitstates
	0x3400-0x37ff = External Data - 4 Waitstates
	0x3800-0x39ff = Internal Data Ram
	0x3a00-0x3bff = Reserved (extra internal ram space on ADSP2101, etc)
	0x3c00-0x3fff = Memory Mapped control registers & reserved.
*/

/* These are the some of the control register, we dont use them all */
enum
{
	S1_AUTOBUF_REG = 15, // 3fef
	S1_RFSDIV_REG,	    // 10   (3ff0)
	S1_SCLKDIV_REG,
	S1_CONTROL_REG,
	S0_AUTOBUF_REG,
	S0_RFSDIV_REG,
	S0_SCLKDIV_REG,
	S0_CONTROL_REG,
	S0_MCTXLO_REG,
	S0_MCTXHI_REG,	// 18
	S0_MCRXLO_REG,
	S0_MCRXHI_REG,	// 1a
	TIMER_SCALE_REG,	// 1b
	TIMER_COUNT_REG,	// 1c
	TIMER_PERIOD_REG,	// 1d
	WAITSTATES_REG,	     	// 1e
	SYSCONTROL_REG	// 1f (3fff)
};


static WRITE16_HANDLER( dcs_control_w )
{
	dcs.control_regs[offset] = data;
//logerror((char *)"dcs_control_w(%X) = %04X (PC=%x)\n", offset, data, adsp2105_get_reg(CPUINFO_INT_PC));
	switch (offset)
	{
		case SYSCONTROL_REG:
			if (data & 0x0200)
			{
//				data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + ((dcs.rombank & 0x7ff) << 12)); 
//				data32_t *dst = (data32_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_PGM_OFFSET);
  				data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE);
				data32_t *dst = (data32_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_PGM_OFFSET);
	
				/* boot force */
//				cpu_set_reset_line(dcs_cpunum, PULSE_LINE);
//				printf("boot force: bank %x %x %x (PC=%x)\n", dcs.rombank, dcs.rombank2, dcs.databank, adsp2105_get_reg(CPUINFO_INT_PC));

				if (dcs.version != DCSRAM)
				{
					adsp2105_load_boot_data(src + (dcs.rombank & 0x7ff) * 0x1000, dst);
					adsp2105_reset(NULL);
				}
				else
				{
					adsp2104_load_boot_data(src + (dcs.databank & 0x7ff) * 0x1000, dst);
					adsp2104_reset(NULL);
				}
				dcs.control_regs[SYSCONTROL_REG] &= ~0x0200;
			}

			/* see if SPORT1 got disabled */
			stream_update(dcs.stream, 0);
			if ((data & 0x0800) == 0)
			{
				dcs.enabled = 0;
//				printf("kill reg timer (SPORT1)\n");
				if (dcs.reg_timer) timer_remove((TimerT *)dcs.reg_timer);
				dcs.reg_timer = NULL;
			}
			break;

		case S1_AUTOBUF_REG:
			/* autobuffer off: nuke the timer, and disable the DAC */
			stream_update(dcs.stream, 0);
			if ((data & 0x0002) == 0)
			{
				dcs.enabled = 0;
//				printf("kill reg timer (autobuffer off)\n");
				if (dcs.reg_timer) timer_remove((TimerT *)dcs.reg_timer);
				dcs.reg_timer = NULL;
			}
			break;

		case S1_CONTROL_REG:
			if (((data >> 4) & 3) == 2)
				logerror((char *)"Oh no!, the data is compresed with u-law encoding\n");
			if (((data >> 4) & 3) == 3)
				logerror((char *)"Oh no!, the data is compresed with A-law encoding\n");
			break;
	}
}



/***************************************************************************
	DCS IRQ GENERATION CALLBACKS
****************************************************************************/

static void dcs_irq(int state)
{
	/* get the index register */
	int reg = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);

	/* translate into data memory bus address */
	int source = ADSP2100_DATA_OFFSET + (reg << 1);
	int i;

//	printf("IRQ, PC = %x\n", cpunum_get_reg(dcs_cpunum, REG_PC));

	/* copy the current data into the buffer */
	for (i = 0; i < dcs.size / 2; i += dcs.incs)
		dcs.buffer[dcs.buffer_in++ & DCS_BUFFER_MASK] = ((UINT16 *)&dcs.mem[source])[i];

	/* increment it */
	reg += dcs.size / 2;

	/* check for wrapping */
	if (reg >= dcs.ireg_base + dcs.size)
	{
		/* reset the base pointer */
		reg = dcs.ireg_base;

		/* generate the (internal, thats why the pulse) irq */
		adsp2105_set_irq_line(ADSP2105_IRQ1, ASSERT_LINE);
		adsp2105_set_irq_line(ADSP2105_IRQ1, CLEAR_LINE);
	}

	/* store it */
	cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, reg);
}


static void sport0_irq(int state)
{
	/* this latches internally, so we just pulse */
	adsp2105_set_irq_line(ADSP2115_SPORT0_RX, ASSERT_LINE);
	adsp2105_set_irq_line(ADSP2115_SPORT0_RX, CLEAR_LINE);

	timer_set(TIME_IN_HZ(1000), 0, sport0_irq); 
}


static void sound_tx_callback(int port, INT32 data)
{
	/* check if it's for SPORT1 */
	if (port != 1)
		return;

	/* check if SPORT1 is enabled */
	if (dcs.control_regs[SYSCONTROL_REG] & 0x0800) /* bit 11 */
	{
		/* we only support autobuffer here (wich is what this thing uses), bail if not enabled */
		if (dcs.control_regs[S1_AUTOBUF_REG] & 0x0002) /* bit 1 */
		{
			/* get the autobuffer registers */
			int		mreg, lreg;
			UINT16	source;
			int		sample_rate;

//			printf("autobuffer enable\n");

			stream_update(dcs.stream, 0);

			dcs.ireg = (dcs.control_regs[S1_AUTOBUF_REG] >> 9) & 7;
			mreg = (dcs.control_regs[S1_AUTOBUF_REG] >> 7) & 3;
			mreg |= dcs.ireg & 0x04; /* msb comes from ireg */
			lreg = dcs.ireg;

			/* now get the register contents in a more legible format */
			/* we depend on register indexes to be continuous (wich is the case in our core) */
			source = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);
			dcs.incs = cpunum_get_reg(dcs_cpunum, ADSP2100_M0 + mreg);
			dcs.size = cpunum_get_reg(dcs_cpunum, ADSP2100_L0 + lreg);
#if DEBUG_DCS
	printf("source = %04X(I%d), incs = %04X(M%d), size = %04X(L%d)\n", source, dcs.ireg, dcs.incs, mreg, dcs.size, lreg);
#endif

			/* get the base value, since we need to keep it around for wrapping */
			source -= dcs.incs;

			/* make it go back one so we dont lose the first sample */
			cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, source);

			/* save it as it is now */
			dcs.ireg_base = source;

			/* get the memory chunk to read the data from */
			dcs.mem = memory_region(REGION_CPU1 + dcs_cpunum);

			/* enable the dac playing */
			dcs.enabled = 1;

			/* calculate how long until we generate an interrupt */

			/* frequency in Hz per each bit sent */
			sample_rate = 10240000 / (2 * (dcs.control_regs[S1_SCLKDIV_REG] + 1));
//			sample_rate = 16000000 / (2 * (2*(dcs.control_regs[S1_SCLKDIV_REG] + 1)));

			/* now put it down to samples, so we know what the channel frequency has to be */
			sample_rate /= 16;

			/* fire off a timer wich will hit every half-buffer */
			dcs.reg_timer = timer_pulse(TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)), 0, dcs_irq);
//			dcs.reg_timer = timer_pulse(TIME_IN_HZ(sample_rate) * (dcs.size / (4 * dcs.incs)), 0, dcs_irq);

			/* configure the DAC generator */
			dcs.sample_step = (int)(sample_rate * 65536.0 / (double)Machine->sample_rate);
			dcs.sample_position = 0;
			dcs.buffer_in = 0;

			return;
		}
		else
			logerror( (char *)"ADSP SPORT1: trying to transmit and autobuffer not enabled!\n" );
	}

	/* if we get there, something went wrong. Disable playing */
	stream_update(dcs.stream, 0);
	dcs.enabled = 0;

	/* remove timer */
	if (dcs.reg_timer) timer_remove((TimerT *)dcs.reg_timer);
	dcs.reg_timer = NULL;
}

static int crusinpfx[] = { 0, 1, 0, -1 };

static void DCS_Init(long srate)
{
	dcs_rw.op_rom = rom_getregion(RGN_CPU1);

	dcs_init(1);

	dcs.version = DCS;

	timer_setnoturbo();

	dcs.input_data = 0;

	// set to full volume (otherwise it's way wimpy and we get horrible
	// speaker pops from the normalizer trying to compensate)
	m1snd_addToCmdQueue(0x55aa);
	m1snd_addToCmdQueue(0xff00);

//	m1snd_addToCmdQueue(0);
//	m1snd_addToCmdQueue(4);
//	m1snd_addToCmdQueue(0);

	// Crusin' needs a prefix
	if (Machine->refcon == 1)
	{
		m1snd_setCmdPrefixStr(crusinpfx);
		m1snd_setNoPrefixOnStop();
	}
}

static void DCS95_Init(long srate)
{
	dcs_rw.op_rom = rom_getregion(RGN_CPU1);

	dcs_init(2);

	dcs.version = DCS95;

	timer_setnoturbo();

	dcs.input_data = 0;

	// set to full volume (otherwise it's way wimpy and we get horrible
	// speaker pops from the normalizer trying to compensate)
	m1snd_addToCmdQueue(0x55aa);
	m1snd_addToCmdQueue(0xff00);
}

static void DCSRAM_Init(long srate)
{
//	unsigned pc = 0x3980, ppc;
//	char buffer[2048];

	dcs_rw.op_rom = rom_getregion(RGN_CPU1);

	dcs.version = DCSRAM;

	dcs_ram_init(0);

	timer_setnoturbo();

	dcs.input_data = 0;

/*	while (pc < 0) //x4000)
	{
		ppc = pc;
		pc += dasm2100(buffer, pc);
		printf("%04x: %s\n", ppc, buffer);
	}*/

	// MK4 needs a value from the host at a certain time to initialize properly
	timer_set(TIME_IN_SEC(0.4), 0, delayed_dcs_w);
}

static void go(int arg)
{
	dcs_data_w(arg);
}

static void DCS_SendCmd(int cmda, int cmdb)
{
	if (Machine->refcon == 1)
	{
		cmda *= 2;
	}

	dcs_data_w((cmda&0xff00)>>8);	// cmd prefix

	timer_set(TIME_IN_USEC(10), cmda&0xff, go);
}

static void DCS_Shutdown(void)
{
	free(dcs.buffer);
}

#if 0
static M1_ROMS_START( dcs2 )
	{
		"Star Wars Episode 1",
		"2000",
		"",
		"swe1",
		MFG_MIDWAY,
		BOARD_DCSROMSTEREO,
		1,
		0,
		ROM_START
			ROM_REGION( ADSP2100_SIZE + 0x800000, RGN_CPU1, 0 )
			ROM_LOAD( "sweflash.snd",  ADSP2100_SIZE + 0x000000, 0x100000, 0x5fc1fd2c )
			ROM_LOAD( "u109.rom",  ADSP2100_SIZE + 0x100000, 0x400000, 0xcc08936b )
			ROM_LOAD( "u110.rom",  ADSP2100_SIZE + 0x500000, 0x400000, 0x6011ecd9 )
		ROM_END
		1, 32768,
	},
	{
		"Mortal Kombat 4",
		"1997",
		"",
		"mk4",
		MFG_MIDWAY,
		BOARD_DCSROMSTEREO,
		1,
		0,
		ROM_START
			ROM_REGION( ADSP2100_SIZE + 0x800000, RGN_CPU1, 0 )
			ROM_LOAD( "mk4_l2.2",  ADSP2100_SIZE + 0x000000, 0x200000, 0x4c01b493 )
			ROM_LOAD( "mk4_l2.3",  ADSP2100_SIZE + 0x200000, 0x200000, 0x8fbcf0ac )
			ROM_LOAD( "mk4_l1.4",  ADSP2100_SIZE + 0x400000, 0x200000, 0xdee91696 )
			ROM_LOAD( "mk4_l1.5",  ADSP2100_SIZE + 0x600000, 0x200000, 0x44d072be )
		ROM_END
		1, 32768,
	},
	{
		"Invasion",
		"1997",
		"",
		"invasn",
		MFG_MIDWAY,
		BOARD_DCSROMSTEREO,
		1,
		0,
		ROM_START
			ROM_REGION( ADSP2100_SIZE + 0x800000, RGN_CPU1, 0 )
			ROM_LOAD( "invau2",  ADSP2100_SIZE + 0x000000, 0x200000, 0x59d2e1d6 )
			ROM_LOAD( "invau3",  ADSP2100_SIZE + 0x200000, 0x200000, 0x86b956ae )
			ROM_LOAD( "invau4",  ADSP2100_SIZE + 0x400000, 0x200000, 0x5ef1fab5 )
			ROM_LOAD( "invau5",  ADSP2100_SIZE + 0x600000, 0x200000, 0xe42805c9 )
		ROM_END
		1, 32768,
	},
	{	
		"Cruisin' Exotica",	// bad dump of ADSP program ROM
		"1996",
		"",
		"crusnex",
		MFG_MIDWAY,
		BOARD_DCSROMSTEREO,
		1,
		0,
		ROM_START
			ROM_REGION( ADSP2100_SIZE + 0xa00000, RGN_CPU1, 0 )
			ROM_LOAD( "exotica.u2",  ADSP2100_SIZE + 0x000000, 0x200000, 0xd2d54acf )
			ROM_LOAD( "exotica.u3",  ADSP2100_SIZE + 0x200000, 0x400000, 0x28a3a13d )
			ROM_LOAD( "exotica.u4",  ADSP2100_SIZE + 0x600000, 0x400000, 0x213f7fd8 )
		ROM_END
		1, 32768,
	},
M1_ROMS_END
#endif

M1_BOARD_START( dcsram )
	MDRV_NAME( "Midway DCS ROM-based stereo" )
	MDRV_HWDESC( "ADSP-2104, DAC" )
//#if (!SPEEDUP_BOOT)
	MDRV_DELAYS( 10000, 75 )
//#else
//	MDRV_DELAYS( 250, 50 )
//#endif
	MDRV_INIT( DCSRAM_Init )
	MDRV_SEND( DCS_SendCmd )
	MDRV_SHUTDOWN( DCS_Shutdown )

#if 0 //SPEEDUP_BOOT	// doesn't apply here
	MDRV_CPU_ADD(ADSP2104, 16000000)
#else	// don't slow down the ram/rom test worse
	MDRV_CPU_ADD(ADSP2104, 16000000)
#endif
	MDRV_CPUMEMHAND(&dcsram_rw)

	MDRV_SOUND_ADD(CUSTOM, &dcs_custom_interface)
M1_BOARD_END

