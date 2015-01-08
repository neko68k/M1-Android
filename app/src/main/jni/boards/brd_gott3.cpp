/* Gottlieb pinball sound boards */

#include "m1snd.h"

static int cmd_latch, ym2151_port, nmi_rate = 0, nmi_enable = 0, u2_latch, u3_latch;
static int enable_w, enable_cs, last_d7, rom_cs;

static READ_HANDLER( latch_r )
{
//	printf("Read %x from latch\n", cmd_latch);
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static READ_HANDLER( latch2_r )
{
//	printf("Slave read %x from latch\n", cmd_latch);
	cpu_set_irq_line(1, 0, CLEAR_LINE);
	return cmd_latch;
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* Guess */
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) },
	{ 0 }
};


static void GT3_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
//	cpu_set_irq_line(1, 0, ASSERT_LINE);
}

static void oki6295_w(void)
{
	if ( enable_cs && enable_w ) {
//		if ( !last_d7 && u2_latch&0x80  )
//			logerror("START OF SAMPLE!\n");

		OKIM6295_data_0_w(0, u2_latch);
//		printf("OKI Data = %x\n", u2_latch);
	}
	else {
		logerror((char *)"NO OKI: cs=%x w=%x\n", enable_cs, enable_w);
	}
	last_d7 = (u2_latch>>7)&1;
}

static WRITE_HANDLER( sound_control_w )
{
	int hold_u3 = u3_latch;
	int hold_enable_w = enable_w;

	/* Bit 7 selects YM2151 register or data port */
	ym2151_port = data & 0x80;
	nmi_enable = data & 0x01;

	u3_latch = (data>>5)&1;
	enable_w = ((~data)>>6)&1;

	if(u3_latch && !hold_u3 ) {

	/*
		U3 - LS374 (Data is fed from the U2 Latch)
	   ----------
		D0 = VUP/DOWN?? - Connects to optional U12 (volume control?)
		D1 = VSTEP??    - Connects to optional U12 (volume control?)
		D2 = 6295 Chip Select (Active Low)
		D3 = ROM Select (0 = Rom1, 1 = Rom2)
		D4 = 6295 - SS (Data = 1 = 8Khz; Data = 0 = 6.4Khz frequency)
		D5 = LED (Active low?)
		D6 = SRET1 (Where is this connected?)
		D7 = SRET2 (Where is this connected?) + /PGM of roms, with optional jumper to +5 volts
	*/

		//D2 = 6295 Chip Select (Active Low)
		enable_cs = ((~u2_latch)>>2)&1;

		//D3 = ROM Select (0 = Rom1, 1 = Rom2)
		rom_cs = (u2_latch>>3)&1;

		//Only swap the rom bank if the value has changed!
		OKIM6295_set_bank_base(0, rom_cs*0x80000);
//		logerror("Setting to rom #%x\n",rom_cs);

		//D4 = 6295 - SS (Data = 1 = 8Khz; Data = 0 = 6.4Khz frequency)
		OKIM6295_set_frequency(0,((u2_latch>>4)&1)?8000:6400);

		//D5 = LED (Active low?)
//		UpdateSoundLEDS(1,~(u2_latch>>5)&1);

		//D6 = SRET1 (Where is this connected?)
		//D7 = SRET2 (Where is this connected?) + /PGM of roms, with optional jumper to +5 volts
	}

	//Trigger Command on Positive Edge
	if (enable_w && !hold_enable_w) oki6295_w();
}

static WRITE_HANDLER( s80bs_ym2151_w )
{
	if (ym2151_port)
		YM2151_data_port_0_w(offset, data);
	else
		YM2151_register_port_0_w(offset, data);
}

static WRITE_HANDLER(s80bs_nmi_rate_w)
{
	nmi_rate = data;
//	logerror("NMI RATE SET TO %d\n",data);
}

static WRITE_HANDLER( u2latch_w )
{
	u2_latch = data;
}

static WRITE_HANDLER(s80bs_cause_dac_nmi_w)
{
//	cpu_set_irq_line(1, IRQ_LINE_NMI, ASSERT_LINE);
//	cpu_set_irq_line(1, IRQ_LINE_NMI, CLEAR_LINE);
}

static READ_HANDLER(s80bs_cause_dac_nmi_r)
{
//	cpu_set_irq_line(1, IRQ_LINE_NMI, ASSERT_LINE);
//	cpu_set_irq_line(1, IRQ_LINE_NMI, CLEAR_LINE);
	return 0;
}

static UINT8 exterm_dac_volume;
static UINT8 exterm_dac_data;

WRITE_HANDLER( exterm_dac_vol_w )
{
	exterm_dac_volume = data ^ 0xff;
	DAC_data_16_w(0, exterm_dac_volume * exterm_dac_data);
}

WRITE_HANDLER( exterm_dac_data_w )
{
	exterm_dac_data = data;
	DAC_data_16_w(0, exterm_dac_volume * exterm_dac_data);
}

struct DACinterface GTS3_dacInt =
{
  2,			/*2 Chips - but it seems we only access 1?*/
 {60,60}		/* Volume */
};

MEMORY_READ_START(GTS3_yreadmem)
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x6800, 0x6800, latch_r },
	{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_ywritemem)
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x4000, 0x4000, s80bs_ym2151_w },
	{ 0x6000, 0x6000, s80bs_nmi_rate_w},
	{ 0x7000, 0x7000, s80bs_cause_dac_nmi_w},
	{ 0x7800, 0x7800, u2latch_w},	// dac latch
	{ 0xa000, 0xa000, sound_control_w },
MEMORY_END

MEMORY_READ_START(GTS3_dreadmem)
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x4000, 0x4000, latch2_r},
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_dwritemem)
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x8000, 0x8000, exterm_dac_vol_w },
	{ 0x8001, 0x8001, exterm_dac_data_w },
MEMORY_END

static void nmi_callback(int param)
{
	//Reset the timing frequency
	double interval;
	int cl1, cl2;
	cl1 = 16-(nmi_rate&0x0f);
	cl2 = 16-((nmi_rate&0xf0)>>4);
	interval = (250000>>8);
	if(cl1>0)	interval /= cl1;
	if(cl2>0)	interval /= cl2;

	//Set up timer to fire again
//	printf("int = %f\n", interval);
	timer_set(TIME_IN_HZ(interval), 0, nmi_callback);

	//If enabled, fire the NMI for the Y CPU
	if(nmi_enable) {
		//logerror("PULSING NMI for Y-CPU\n");
		//timer_set(TIME_NOW, 1, NULL);
//		printf("Y-CPU NMI\n");
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	}
}

static void GT3_Init(long srate)
{
	nmi_enable = 0;
	nmi_rate = 0;
	ym2151_port = 0;
	enable_w = 0;
	enable_cs = 0;

	timer_set(1.0/60.0, 0, nmi_callback);
}

struct OKIM6295interface GTS3_okim6295_interface = {
	1,						/* 1 chip */
	{ 8000 },				/* 8000Hz frequency */
	{ RGN_SAMP1 },	/* memory region */
	{ 35 }
};

M1_BOARD_START( gotts3 )
	MDRV_NAME("Gottlieb System 3")
	MDRV_HWDESC("6502(x2), YM2151, MSM-6295, DAC")
	MDRV_INIT( GT3_Init )
	MDRV_SEND( GT3_SendCmd )

	MDRV_CPU_ADD(M6502B, 2000000)
	MDRV_CPU_MEMORY(GTS3_yreadmem,GTS3_ywritemem)

	MDRV_CPU_ADD(M6502B, 2000000)
	MDRV_CPU_MEMORY(GTS3_dreadmem,GTS3_dwritemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &GTS3_okim6295_interface )
	MDRV_SOUND_ADD(DAC, &GTS3_dacInt )
M1_BOARD_END

