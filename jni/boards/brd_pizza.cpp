// brd_pizza: Gaelco 3D games

#include "m1snd.h"

#define SOUND_CHANNELS (4)

extern "C" {
unsigned dasm2100(char *buffer, unsigned pc);
void adsp2104_reset(void *param);
void adsp2105_reset(void *param);
void adsp2115_get_info(UINT32 state, union cpuinfo *info);
void adsp2115_set_info(UINT32 state, union cpuinfo *info);
}

static void G3D_Init(long srate);
static void G3D_SendCmd(int cmda, int cmdb);

static int latch_data;
static INT32 rombnk;

static unsigned int g3d_read8(unsigned int address);
static unsigned int g3d_read16(unsigned int address);
static unsigned int g3d_read32(unsigned int address);
static void g3d_write8(unsigned int address, unsigned int data);
static void g3d_write16(unsigned int address, unsigned int data);
static void g3d_write32(unsigned int address, unsigned int data);

static union cpuinfo info;

static unsigned int g3dcpunum_get_reg(int cpu, int reg) 
{ 
	adsp2115_get_info(CPUINFO_INT_REGISTER + reg, &info); 
	return info.i;
}

#define adsp2105_get_reg(x) g3dcpunum_get_reg(0, x)
#define cpunum_set_reg(cpu, reg, val) { info.i = val; adsp2115_set_info(CPUINFO_INT_REGISTER + reg, &info); }

#define adsp2105_set_irq_line(line, val) { info.i = val; adsp2115_set_info(CPUINFO_INT_INPUT_STATE + line, &info); }

typedef INT32 (*RX_CALLBACK)( int port );
typedef void  (*TX_CALLBACK)( int port, INT32 data );

static void *adsp_autobuffer_timer;
static UINT16 adsp_control_regs[(0x4000-0x3fe0)];
static UINT8 adsp_ireg;
static offs_t adsp_ireg_base, adsp_incs, adsp_size;

static void adsp2105_set_tx_callback(TX_CALLBACK cb)
{
	info.p = (void *)cb;
	adsp2115_set_info(CPUINFO_PTR_ADSP2100_TX_HANDLER, &info);
}

M1ADSPT g3d_rw =
{
	g3d_read8,
	g3d_read16,
	g3d_read32,
	g3d_write8,
	g3d_write16,
	g3d_write32
};

enum
{
	S1_AUTOBUF_REG = 15,
	S1_RFSDIV_REG,
	S1_SCLKDIV_REG,
	S1_CONTROL_REG,
	S0_AUTOBUF_REG,
	S0_RFSDIV_REG,
	S0_SCLKDIV_REG,
	S0_CONTROL_REG,
	S0_MCTXLO_REG,
	S0_MCTXHI_REG,
	S0_MCRXLO_REG,
	S0_MCRXHI_REG,
	TIMER_SCALE_REG,
	TIMER_COUNT_REG,
	TIMER_PERIOD_REG,
	WAITSTATES_REG,
	SYSCONTROL_REG
};

static void adsp_autobuffer_irq(int state)
{
	/* get the index register */
	int reg = g3dcpunum_get_reg(2, ADSP2100_I0 + adsp_ireg);

	/* copy the current data into the buffer */
//	printf("ADSP buffer: I%d=%04X incs=%04X size=%04X\n", adsp_ireg, reg, adsp_incs, adsp_size);
	if (adsp_incs)
		dmadac_transfer(0, SOUND_CHANNELS, adsp_incs, SOUND_CHANNELS * adsp_incs, adsp_size / (SOUND_CHANNELS * adsp_incs), (INT16 *)&prgrom[reg - 0x3800 + ADSP2100_DATA_OFFSET]);

	/* increment it */
	reg += adsp_size;

	/* check for wrapping */
	if ((unsigned int)reg >= adsp_ireg_base + adsp_size)
	{
		/* reset the base pointer */
		reg = adsp_ireg_base;

		/* generate the (internal, thats why the pulse) irq */
		adsp2105_set_irq_line(ADSP2105_IRQ1, ASSERT_LINE);
		adsp2105_set_irq_line(ADSP2105_IRQ1, CLEAR_LINE);
	}

	/* store it */
	cpunum_set_reg(2, ADSP2100_I0 + adsp_ireg, reg);
}


static void adsp_tx_callback(int port, INT32 data)
{
	/* check if it's for SPORT1 */
	if (port != 1)
		return;

	/* check if SPORT1 is enabled */
 	if (adsp_control_regs[SYSCONTROL_REG] & 0x0800) /* bit 11 */
	{
		/* we only support autobuffer here (wich is what this thing uses), bail if not enabled */
		if (adsp_control_regs[S1_AUTOBUF_REG] & 0x0002) /* bit 1 */
		{
			/* get the autobuffer registers */
			int		mreg, lreg;
			UINT16	source;
			double	sample_rate;

			adsp_ireg = (adsp_control_regs[S1_AUTOBUF_REG] >> 9) & 7;
			mreg = (adsp_control_regs[S1_AUTOBUF_REG] >> 7) & 3;
			mreg |= adsp_ireg & 0x04; /* msb comes from ireg */
			lreg = adsp_ireg;

			/* now get the register contents in a more legible format */
			/* we depend on register indexes to be continuous (wich is the case in our core) */
			source = g3dcpunum_get_reg(2, ADSP2100_I0 + adsp_ireg);
			adsp_incs = g3dcpunum_get_reg(2, ADSP2100_M0 + mreg);
			adsp_size = g3dcpunum_get_reg(2, ADSP2100_L0 + lreg);

			/* get the base value, since we need to keep it around for wrapping */
			source -= adsp_incs;

			/* make it go back one so we dont lose the first sample */
			cpunum_set_reg(2, ADSP2100_I0 + adsp_ireg, source);

			/* save it as it is now */
			adsp_ireg_base = source;

			/* calculate how long until we generate an interrupt */

			/* frequency in Hz per each bit sent */
			sample_rate = (double)16000000.0f / (double)(2 * (adsp_control_regs[S1_SCLKDIV_REG] + 1));

			/* now put it down to samples, so we know what the channel frequency has to be */
			sample_rate /= 16 * SOUND_CHANNELS;
 			printf("sample_rate = %f\n", sample_rate);
			dmadac_set_frequency(0, SOUND_CHANNELS, sample_rate);
			dmadac_enable(0, SOUND_CHANNELS, 1);

			/* fire off a timer wich will hit every half-buffer */
			timer_adjust(adsp_autobuffer_timer, TIME_IN_HZ(sample_rate) * (adsp_size / (SOUND_CHANNELS * adsp_incs)), 0, TIME_IN_HZ(sample_rate) * (adsp_size / (SOUND_CHANNELS * adsp_incs)));

			return;
		}
		else
			logerror( (char *)"ADSP SPORT1: trying to transmit and autobuffer not enabled!\n" );
	}

	/* if we get there, something went wrong. Disable playing */
	dmadac_enable(0, SOUND_CHANNELS, 0);

	/* remove timer */
	timer_adjust(adsp_autobuffer_timer, TIME_NEVER, 0, 0);
}

static int suffix[6] = { 0xff, 0xe8, 0x80, 0x7c, 0x7f, -1 };

static void G3D_Init(long srate)
{
	UINT16 *src;
	int i;

	adsp2105_set_tx_callback(adsp_tx_callback);

	/* boot the ADSP chip */
	src = (UINT16 *)(memory_region(REGION_CPU1)+0x20000);
	for (i = 0; i < (src[3] & 0xff) * 8; i++)
	{
		UINT32 opcode = ((src[i*4+0] & 0xff) << 16) | ((src[i*4+1] & 0xff) << 8) | (src[i*4+2] & 0xff);
		ADSP2100_WRPGM(&prgrom[ADSP2100_PGM_OFFSET + (i<<2)], opcode);
	}

	/* allocate a timer for feeding the autobuffer */
	adsp_autobuffer_timer = timer_alloc(adsp_autobuffer_irq);

	m1snd_setCmdPrefix(0x20);
	m1snd_setCmdSuffixStr(suffix);

	rombnk = 0;
}

static void G3D_SendCmd(int cmda, int cmdb)
{
	latch_data = cmda;
	adsp2105_set_irq_line(ADSP2105_IRQ2, ASSERT_LINE);
}

static unsigned int g3d_read8(unsigned int address)
{
	return prgrom[address];
}

static unsigned int g3d_read16(unsigned int address)
{
	int daddress = (address >> 1);

	if (daddress <= 0x1fff)
	{
		return mem_readword((unsigned short *)(prgrom+address+rombnk));
	}

	if (daddress == 0x2000) 
	{
		printf("Reading %x @ latch\n", latch_data);
		adsp2105_set_irq_line(ADSP2105_IRQ2, CLEAR_LINE);
		return latch_data;
	}

	return mem_readword((unsigned short *)(prgrom+address));
}

static unsigned int g3d_read32(unsigned int address)
{
	return mem_readlong((unsigned int *)(&prgrom[address+ADSP2100_PGM_OFFSET]));
}

static void g3d_write8(unsigned int address, unsigned int data)
{
  	prgrom[address] = data;
}

static void g3d_write16(unsigned int address, unsigned int data)
{
	int daddress;
	
	daddress = (address >> 1);

	if (daddress <= 1)
	{
		rombnk = ((daddress & 1) * 0x80 + (data & 0x7f)) * 0x4000;

		printf("ROM bank select: offset %x val %02x => [%x]\n", daddress, data, rombnk);
		return;
	}

	if (daddress == 0x2000)
	{
		// status_w
//		printf("%04x to status\n", data);
		return;
	}

	if (daddress >= 0x3fe0 && daddress <= 0x3fff)
	{
		adsp_control_regs[daddress-0x3fe0] = data;
		return;
	}

	mem_writeword((unsigned short *)(prgrom+address), data);
}

static void g3d_write32(unsigned int address, unsigned int data)
{
	mem_writelong((unsigned int *)(&prgrom[address+ADSP2100_PGM_OFFSET]), data);
}

struct dmadac_interface dac_interface =
{
	4,
	{ MIXER(100, MIXER_PAN_CENTER), MIXER(100, MIXER_PAN_CENTER), MIXER(100, MIXER_PAN_CENTER), MIXER(100, MIXER_PAN_CENTER) }
};

M1_BOARD_START( gaelco3d )
	MDRV_NAME( "Gaelco 3D" )
	MDRV_HWDESC( "ADSP-2115, DAC" )

	MDRV_DELAYS( 1000, 40 )

	MDRV_INIT( G3D_Init )
	MDRV_SEND( G3D_SendCmd )

	MDRV_CPU_ADD(ADSP2115, 16000000)

	MDRV_CPUMEMHAND(&g3d_rw)

	MDRV_SOUND_ADD(DMADAC, &dac_interface)
M1_BOARD_END

