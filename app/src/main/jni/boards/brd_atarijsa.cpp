/* Atari JSA-series boards */

#include "m1snd.h"

#define ATARI_CLOCK_3MHz	 3579000
#define ATARI_CLOCK_20MHz	20000000
#define M6502_CLOCK (ATARI_CLOCK_3MHz/2)
#define POKEY_CLOCK (ATARI_CLOCK_3MHz/2)
#define YM_CLOCK (ATARI_CLOCK_3MHz)
#define TMS_CLOCK (ATARI_CLOCK_3MHz*2/11)
#define OKI_CLOCK (ATARI_CLOCK_3MHz/3/132)

#define TIMER_RATE (0.004005)	// taken from system 1

void atarigen_ym2151_irq_gen(int irq);

static int cmd_latch, status = 0;
static int tms5220_data, tms5220_data_strobe, last_ctl, has_tms5220, oki6295_bank_base;

static unsigned char *bank_base;
static unsigned char *bank_source_data;

static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_3MHz*2/11,	/* potentially ATARI_CLOCK_3MHz/9 as well */
	100,
	0
};

static struct POKEYinterface pokey_interface =
{
	1,			/* 1 chip */
	ATARI_CLOCK_3MHz/2,
	{ 40 },
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	ATARI_CLOCK_3MHz,
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ atarigen_ym2151_irq_gen }
};


static struct YM2151interface ym2151_interface_swapped =
{
	1,			/* 1 chip */
	ATARI_CLOCK_3MHz,
	{ YM3012_VOL(60,MIXER_PAN_RIGHT,60,MIXER_PAN_LEFT) },
	{ atarigen_ym2151_irq_gen }
};


static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ ATARI_CLOCK_3MHz/3/132 },
	{ RGN_SAMP1 },
	{ 75 }
};


static struct OKIM6295interface okim6295s_interface =
{
	2, 				/* 2 chips */
	{ ATARI_CLOCK_3MHz/3/132, ATARI_CLOCK_3MHz/3/132 },
	{ RGN_SAMP1, RGN_SAMP1 },
	{ MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) }
};

void atarigen_ym2151_irq_gen(int irq)
{
	if (irq)
		cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);
}

// JSA1 I/O handlers
static READ_HANDLER( jsa1_io_r )
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* n/c */
			break;

		case 0x002:		/* /RDP */
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
			status &= ~0x40;
			result = cmd_latch;
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test
				0x40 = NMI line state (active low)
				0x20 = sound output full
				0x10 = TMS5220 ready (active low)
				0x08 = +5V
				0x04 = +5V
				0x02 = coin 2
				0x01 = coin 1
			*/
	//				result = readinputport(input_port);
	//				if (!(readinputport(test_port) & test_mask)) result ^= 0x80;
	//				if (status) result ^= 0x40;
	//				if (atarigen_sound_to_cpu_ready) result ^= 0x20; 
			result = status;
			if (!has_tms5220 || tms5220_ready_r()) result ^= 0x10;
//			printf("rd status %x\n", result);
			break;

		case 0x006:		/* /IRQACK */
			break;

		case 0x200:		/* /VOICE */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			break;
	}

	return result;
}

static WRITE_HANDLER( jsa1_io_w )
{
	switch (offset & 0x206)
	{
		case 0x000:		/* n/c */
		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
			break;

		case 0x006:		/* /IRQACK */
			break;

		case 0x200:		/* /VOICE */
			tms5220_data = data;
			break;

		case 0x202:		/* /WRP */
//			printf("6502 sending %x to 68010\n", data);
			break;

		case 0x204:		/* WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = squeak (tweaks the 5220 frequency)
				0x04 = TMS5220 reset (active low)
				0x02 = TMS5220 write strobe (active low)
				0x01 = YM2151 reset (active low)
			*/

			/* handle TMS5220 I/O */
			if (has_tms5220)
			{
				int count;

				if (((data ^ last_ctl) & 0x02) && (data & 0x02))
					tms5220_data_w(0, tms5220_data);
				count = 5 | ((data >> 2) & 2);
				tms5220_set_frequency(ATARI_CLOCK_3MHz*2 / (16 - count));
			}

			/* update the bank */
			memcpy(bank_base, &bank_source_data[0x1000 * ((data >> 6) & 3)], 0x1000);
			last_ctl = data;
			break;

		case 0x206:		/* MIX */
			/*
				0xc0 = TMS5220 volume (0-3)
				0x30 = POKEY volume (0-3)
				0x0e = YM2151 volume (0-7)
				0x01 = low-pass filter enable
			*/
	//				tms5220_volume = ((data >> 6) & 3) * 100 / 3;
	//				pokey_volume = ((data >> 4) & 3) * 100 / 3;
	//				ym2151_volume = ((data >> 1) & 7) * 100 / 7;
	//				update_all_volumes();
			break;
	}
}

// JSA2 I/O handlers
static READ_HANDLER( jsa2_io_r )
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
			result = OKIM6295_status_0_r(offset);
			break;

		case 0x002:		/* /RDP */
	//				printf("reading cmd %d\n", cmd_latch);
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
			status &= ~0x40;
			result = cmd_latch;
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test
				0x40 = NMI line state (active low)
				0x20 = sound output full
				0x10 = +5V
				0x08 = +5V
				0x04 = +5V
				0x02 = coin 2
				0x01 = coin 1
			*/
//			result = readinputport(input_port);
//			if (!(readinputport(test_port) & test_mask)) result ^= 0x80;
//			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
//			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			result = status;
			break;

		case 0x006:		/* /IRQACK */
			break;

		case 0x200:		/* /WRV */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			break;
	}

	return result;
}


static WRITE_HANDLER( jsa2_io_w )
{
	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
			logerror((char *)"atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
			break;

		case 0x200:		/* /WRV */
			OKIM6295_data_0_w(offset, data);
			break;

		case 0x202:		/* /WRP */
			break;

		case 0x204:		/* /WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = voice frequency (tweaks the OKI6295 frequency)
				0x04 = OKI6295 reset (active low)
				0x02 = n/c
				0x01 = YM2151 reset (active low)
			*/

			/* update the bank */
			memcpy(bank_base, &bank_source_data[0x1000 * ((data >> 6) & 3)], 0x1000);
			last_ctl = data;

			/* update the OKI frequency */
//			OKIM6295_set_frequency(0, ATARI_CLOCK_3MHz/3 / ((data & 8) ? 132 : 165), Machine->sample_rate);
			break;

		case 0x206:		/* /MIX */
			/*
				0xc0 = n/c
				0x20 = low-pass filter enable
				0x10 = n/c
				0x0e = YM2151 volume (0-7)
				0x01 = OKI6295 volume (0-1)
			*/
//			ym2151_volume = ((data >> 1) & 7) * 100 / 7;
//			oki6295_volume = 50 + (data & 1) * 50;
//			update_all_volumes();
			break;
	}
}

// JSA3 handler
static WRITE_HANDLER( jsa3_io_w )
{
	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
//			overall_volume = data * 100 / 127;
//			update_all_volumes();
			break;

		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
//			logerror("atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
//			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /WRV */
			OKIM6295_data_0_w(offset, data);
			break;

		case 0x202:		/* /WRP */
			break;

		case 0x204:		/* /WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = voice frequency (tweaks the OKI6295 frequency)
				0x04 = OKI6295 reset (active low)
				0x02 = OKI6295 bank bit 0
				0x01 = YM2151 reset (active low)
			*/

			/* update the OKI bank */
			oki6295_bank_base = (0x40000 * ((data >> 1) & 1)) | (oki6295_bank_base & 0x80000);
//			OKIM6295_set_bank_base(0, oki6295_bank_base);

			/* update the bank */
			memcpy(bank_base, &bank_source_data[0x1000 * ((data >> 6) & 3)], 0x1000);
			last_ctl = data;

			/* update the OKI frequency */
//			OKIM6295_set_frequency(0, ATARI_CLOCK_3MHz/3 / ((data & 8) ? 132 : 165), Machine->sample_rate);
			break;

		case 0x206:		/* /MIX */
			/*
				0xc0 = n/c
				0x20 = low-pass filter enable
				0x10 = OKI6295 bank bit 1
				0x0e = YM2151 volume (0-7)
				0x01 = OKI6295 volume (0-1)
			*/

			/* update the OKI bank */
			oki6295_bank_base = (0x80000 * ((data >> 4) & 1)) | (oki6295_bank_base & 0x40000);
//			OKIM6295_set_bank_base(0, oki6295_bank_base);

			/* update the volumes */
#if 0
			ym2151_volume = ((data >> 1) & 7) * 100 / 7;
			oki6295_volume = 50 + (data & 1) * 50;
			update_all_volumes();
#endif
			break;
	}
}

// JSA3S handlers
static READ_HANDLER( jsa3s_io_r )
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
			if (offset & 1)
				result = OKIM6295_status_1_r(offset); 
			else
				result = OKIM6295_status_0_r(offset);
			break;

		case 0x002:		/* /RDP */
	//				printf("reading cmd %d\n", cmd_latch);
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
			status &= ~0x40;
			result = cmd_latch;
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test
				0x40 = NMI line state (active low)
				0x20 = sound output full
				0x10 = +5V
				0x08 = +5V
				0x04 = +5V
				0x02 = coin 2
				0x01 = coin 1
			*/
//			result = readinputport(input_port);
//			if (!(readinputport(test_port) & test_mask)) result ^= 0x80;
//			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
//			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			result = status;
			break;

		case 0x006:		/* /IRQACK */
			break;

		case 0x200:		/* /WRV */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			break;
	}

	return result;
}

static WRITE_HANDLER( jsa3s_io_w )
{
	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
//			overall_volume = data * 100 / 127;
//			update_all_volumes();
			break;

		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
//			logerror("atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
//			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /WRV */
			if (offset & 1)
				OKIM6295_data_1_w(offset, data);
			else
				OKIM6295_data_0_w(offset, data);
			break;

		case 0x202:		/* /WRP */
//			atarigen_6502_sound_w(offset, data);
			break;

		case 0x204:		/* /WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = voice frequency (tweaks the OKI6295 frequency)
				0x04 = OKI6295 reset (active low)
				0x02 = left OKI6295 bank bit 0
				0x01 = YM2151 reset (active low)
			*/

			/* update the OKI bank */
			oki6295_bank_base = (0x40000 * ((data >> 1) & 1)) | (oki6295_bank_base & 0x80000);
//			OKIM6295_set_bank_base(0, oki6295_bank_base);

			/* update the bank */
			memcpy(bank_base, &bank_source_data[0x1000 * ((data >> 6) & 3)], 0x1000);
			last_ctl = data;

			/* update the OKI frequency */
//			OKIM6295_set_frequency(0, ATARI_CLOCK_3MHz/3 / ((data & 8) ? 132 : 165), Machine->sample_rate);
//			OKIM6295_set_frequency(1, ATARI_CLOCK_3MHz/3 / ((data & 8) ? 132 : 165), Machine->sample_rate);
			break;

		case 0x206:		/* /MIX */
			/*
				0xc0 = right OKI6295 bank bits 0-1
				0x20 = low-pass filter enable
				0x10 = left OKI6295 bank bit 1
				0x0e = YM2151 volume (0-7)
				0x01 = OKI6295 volume (0-1)
			*/

			/* update the OKI bank */
			oki6295_bank_base = (0x80000 * ((data >> 4) & 1)) | (oki6295_bank_base & 0x40000);
//			OKIM6295_set_bank_base(0, oki6295_bank_base);
//			OKIM6295_set_bank_base(1, 0x40000 * (data >> 6));

			/* update the volumes */
//			ym2151_volume = ((data >> 1) & 7) * 100 / 7;
//			oki6295_volume = 50 + (data & 1) * 50;
//			update_all_volumes();
			break;
	}
}
 
static void timer(int ref)
{
	cpu_set_irq_line(0, M6502_IRQ_LINE, ASSERT_LINE);
	cpu_set_irq_line(0, M6502_IRQ_LINE, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, timer);
}

static void jsa1_base_init(void)
{
	tms5220_data_strobe = 1;

	// copy the non-banked portion of the program down
	memcpy(&prgrom[0x4000], &prgrom[0x14000], 0xc000);

	// clear workram
	memset(&workram[0x1000], 0xff, 0x800);

	bank_base = &prgrom[0x3000];
	bank_source_data = &prgrom[0x10000];

	// Guardians of the Hood assumes bank 0 on boot
	memcpy(bank_base, &bank_source_data[0x0000], 0x1000);

	timer_set(TIMER_RATE, 0, timer);

	has_tms5220 = 0;
	tms5220_data = 0;
	last_ctl = 0;
}

static void JSA_Init(long srate)
{
	jsa1_base_init();
}

static void JSA_InitSpeech(long srate)
{
	jsa1_base_init();

	has_tms5220 = 1;
}

static void JSA2_Init(long srate)
{
	jsa1_base_init();

	OKIM6295_set_bank_base(0, 0);
}

static void JSA3_Init(long srate)
{
	unsigned char *base = rom_getregion(RGN_SAMP1);

	jsa1_base_init();

	/* expand the ADPCM data to avoid lots of memcpy's during gameplay */
	/* the upper 128k is fixed, the lower 128k is bankswitched */
	memcpy(&base[0x00000], &base[0x80000], 0x20000);
	memcpy(&base[0x40000], &base[0x80000], 0x20000);
	memcpy(&base[0x80000], &base[0xa0000], 0x20000);

	memcpy(&base[0x20000], &base[0xe0000], 0x20000);
	memcpy(&base[0x60000], &base[0xe0000], 0x20000);
	memcpy(&base[0xa0000], &base[0xe0000], 0x20000);

	OKIM6295_set_bank_base(0, 0);
}

static void JSA3S_Init(long srate)
{
	unsigned char *base = rom_getregion(RGN_SAMP1);

	jsa1_base_init();

	/* expand the ADPCM data to avoid lots of memcpy's during gameplay */
	/* the upper 128k is fixed, the lower 128k is bankswitched */
	memcpy(&base[0x00000], &base[0x80000], 0x20000);
	memcpy(&base[0x40000], &base[0x80000], 0x20000);
	memcpy(&base[0x80000], &base[0xa0000], 0x20000);

	memcpy(&base[0x20000], &base[0xe0000], 0x20000);
	memcpy(&base[0x60000], &base[0xe0000], 0x20000);
	memcpy(&base[0xa0000], &base[0xe0000], 0x20000);

	OKIM6295_set_bank_base(0, 0);
	OKIM6295_set_bank_base(1, 0);
}

static void JSA_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	status |= 0x40;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

MEMORY_READ_START( jsa1_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2800, 0x2bff, jsa1_io_r },
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x3000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( jsa1_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2bff, jsa1_io_w },
	{ 0x2c00, 0x2c0f, pokey1_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END

M1_BOARD_START( jsa1_stereo )
	MDRV_NAME("JSA-1")
	MDRV_HWDESC("6502, YM2151, POKEY")
	MDRV_INIT( JSA_Init )
	MDRV_SEND( JSA_SendCmd )
	MDRV_DELAYS( 800, 50 )

	MDRV_CPU_ADD(M6502B, M6502_CLOCK)
	MDRV_CPU_MEMORY(jsa1_readmem,jsa1_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(POKEY, &pokey_interface)
M1_BOARD_END

M1_BOARD_START( jsa1_swap )
	MDRV_NAME("JSA-1 (inverted stereo)")
	MDRV_HWDESC("6502, YM2151, POKEY")
	MDRV_INIT( JSA_Init )
	MDRV_SEND( JSA_SendCmd )
	MDRV_DELAYS( 800, 50 )

	MDRV_CPU_ADD(M6502B, M6502_CLOCK)
	MDRV_CPU_MEMORY(jsa1_readmem,jsa1_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface_swapped)
	MDRV_SOUND_ADD(POKEY, &pokey_interface)
M1_BOARD_END

M1_BOARD_START( jsa1_mono_speech )
	MDRV_NAME("JSA-1 (w/speech)")
	MDRV_HWDESC("6502, YM2151, TMS5220")
	MDRV_INIT( JSA_InitSpeech )
	MDRV_SEND( JSA_SendCmd )
	MDRV_DELAYS( 800, 50 )

	MDRV_CPU_ADD(M6502B, M6502_CLOCK)
	MDRV_CPU_MEMORY(jsa1_readmem,jsa1_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(POKEY, &pokey_interface)
	MDRV_SOUND_ADD(TMS5220, &tms5220_interface)
M1_BOARD_END

MEMORY_READ_START( jsa2_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2800, 0x2bff, jsa2_io_r },
	{ 0x3000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( jsa2_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2bff, jsa2_io_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END

M1_BOARD_START( jsa2 )
	MDRV_NAME("JSA-2")
	MDRV_HWDESC("6502, YM2151, MSM-6295")
	MDRV_INIT( JSA2_Init )
	MDRV_SEND( JSA_SendCmd )
	MDRV_DELAYS( 800, 50 )

	MDRV_CPU_ADD(M6502B, M6502_CLOCK)
	MDRV_CPU_MEMORY(jsa2_readmem,jsa2_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

MEMORY_READ_START( jsa3_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2800, 0x2bff, jsa3s_io_r },
	{ 0x3000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( jsa3_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2bff, jsa3_io_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END

M1_BOARD_START( jsa3 )
	MDRV_NAME("JSA-3")
	MDRV_HWDESC("6502, YM2151, MSM-6295")
	MDRV_INIT( JSA3_Init )
	MDRV_SEND( JSA_SendCmd )
	MDRV_DELAYS( 800, 50 )

	MDRV_CPU_ADD(M6502B, M6502_CLOCK)
	MDRV_CPU_MEMORY(jsa3_readmem,jsa3_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

MEMORY_READ_START( jsa3s_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2800, 0x2bff, jsa3s_io_r },
	{ 0x3000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( jsa3s_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2bff, jsa3s_io_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END

M1_BOARD_START( jsa3s )
	MDRV_NAME("JSA-3S")
	MDRV_HWDESC("6502, YM2151, MSM-6295(x2)")
	MDRV_INIT( JSA3S_Init )
	MDRV_SEND( JSA_SendCmd )
	MDRV_DELAYS( 800, 50 )

	MDRV_CPU_ADD(M6502B, M6502_CLOCK)
	MDRV_CPU_MEMORY(jsa3s_readmem,jsa3s_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295s_interface)
M1_BOARD_END

