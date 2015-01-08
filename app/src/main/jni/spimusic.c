#include "driver.h"
#include "cpuintrf.h"
#include "sound/ymf271.h"

/********************************************************************/
static int z80_lastbank;

#define FIFO_SIZE 512
static int fifoin_rpos, fifoin_wpos;
static UINT8 fifoin_data[FIFO_SIZE];
static int fifoin_read_request = 0;

static int fifoout_rpos, fifoout_wpos;
static UINT8 fifoout_data[FIFO_SIZE];
static int fifoout_read_request = 0;

static UINT8 z80_fifoout_pop(void)
{
	UINT8 r;
	if (fifoout_wpos == fifoout_rpos)
	{
		printf("Sound FIFOOUT underflow at %08X\n", activecpu_get_pc());
	}
	r = fifoout_data[fifoout_rpos++];
	if(fifoout_rpos == FIFO_SIZE)
	{
		fifoout_rpos = 0;
	}

	if (fifoout_wpos == fifoout_rpos)
	{
		fifoout_read_request = 0;
	}

	return r;
}

static void z80_fifoout_push(UINT8 data)
{
	fifoout_data[fifoout_wpos++] = data;
	if (fifoout_wpos == FIFO_SIZE)
	{
		fifoout_wpos = 0;
	}
	if(fifoout_wpos == fifoout_rpos)
	{
		osd_die("Sound FIFOOUT overflow at %08X\n", activecpu_get_pc());
	}

	fifoout_read_request = 1;
}

static UINT8 z80_fifoin_pop(void)
{
	UINT8 r;
	if (fifoin_wpos == fifoin_rpos)
	{
		osd_die("Sound FIFOIN underflow at %08X\n", activecpu_get_pc());
	}
	r = fifoin_data[fifoin_rpos++];
	if(fifoin_rpos == FIFO_SIZE)
	{
		fifoin_rpos = 0;
	}

	if (fifoin_wpos == fifoin_rpos)
	{
		fifoin_read_request = 0;
	}

	return r;
}

static void z80_fifoin_push(UINT8 data)
{
	fifoin_data[fifoin_wpos++] = data;
	if(fifoin_wpos == FIFO_SIZE)
	{
		fifoin_wpos = 0;
	}
	if(fifoin_wpos == fifoin_rpos)
	{
		osd_die("Sound FIFOIN overflow at %08X\n", activecpu_get_pc());
	}

	fifoin_read_request = 1;
}



static READ32_HANDLER( sound_fifo_r )
{
	UINT8 r = z80_fifoout_pop();

	return r;
}

static WRITE32_HANDLER( sound_fifo_w )
{
	if( (mem_mask & 0xff) == 0 ) {
		z80_fifoin_push(data & 0xff);
	}
}

static READ32_HANDLER( sound_fifo_status_r )
{
	UINT32 r = 0;
	if (fifoout_read_request)
	{
		r |= 2;
	}
	return r | 1;
}

/********************************************************************/

VIDEO_START( spimusic )
{
	return 0;	
}

UINT16 track = 0x8000;
int tick = 0;

VIDEO_UPDATE( spimusic )
{
	char string[200];
	fillbitmap(bitmap, 0, cliprect);
	
	sprintf(string, "Code: %04X\n", track);
	ui_text(bitmap, string, 50, 50);
	
	tick++;
	if (tick >= 5)
	{
		tick = 0;
		if (code_pressed(KEYCODE_E))
			track++;
		if (code_pressed(KEYCODE_Q))
			track--;
			
		if (track < 0x8000)
			track = 0x8000;
			
		if (code_pressed(KEYCODE_T) )
		{
			z80_fifoin_push(0x93);
			z80_fifoin_push(0xff);
			z80_fifoin_push((track >> 8) & 0xff);
			z80_fifoin_push((track >> 0) & 0xff);
		}
	}
}



static READ8_HANDLER( z80_soundfifo_r )
{
	UINT8 r = z80_fifoin_pop();

	return r;
}

static WRITE8_HANDLER( z80_soundfifo_w )
{
	z80_fifoout_push(data);
}

static READ8_HANDLER( z80_soundfifo_status_r )
{
	UINT8 r = 0;
	if (fifoin_read_request)
	{
		r |= 2;
	}
	return r | 1;
}

static WRITE8_HANDLER( z80_bank_w )
{
	if ((data & 7) != z80_lastbank)
	{
		z80_lastbank = (data & 7);
		memory_set_bankptr(4, memory_region(REGION_CPU1) + (0x8000 * z80_lastbank));
	}
}

static READ8_HANDLER( z80_jp1_r )
{
	//return readinputport(3);
	return 0xff;
}

static READ8_HANDLER( z80_coin_r )
{
	//return readinputport(4);
	return 0xff;
}

/********************************************************************/

static ADDRESS_MAP_START( spisound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4008, 0x4008) AM_READ(z80_soundfifo_r)
	AM_RANGE(0x4009, 0x4009) AM_READ(z80_soundfifo_status_r)
	AM_RANGE(0x400a, 0x400a) AM_READ(z80_jp1_r)
	AM_RANGE(0x4013, 0x4013) AM_READ(z80_coin_r)
	AM_RANGE(0x6000, 0x600f) AM_READ(YMF271_0_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK4)		/* Banked ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( spisound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4002, 0x4002) AM_WRITE(MWA8_NOP)		/* ack RST 10 */
	AM_RANGE(0x4003, 0x4003) AM_WRITE(MWA8_NOP)		/* Unknown */
	AM_RANGE(0x4008, 0x4008) AM_WRITE(z80_soundfifo_w)
	AM_RANGE(0x400b, 0x400b) AM_WRITE(MWA8_NOP)		/* Unknown */
	AM_RANGE(0x401b, 0x401b) AM_WRITE(z80_bank_w)		/* control register: bits 0-2 = bank @ 8000, bit 3 = watchdog? */
	AM_RANGE(0x6000, 0x600f) AM_WRITE(YMF271_0_w)
ADDRESS_MAP_END

static READ8_HANDLER( flashrom_read )
{
	UINT8 *flashrom = memory_region(REGION_SOUND1);
	logerror("Flash Read: %08X\n", offset);
	
	return flashrom[offset & 0x1fffff];
}

static WRITE8_HANDLER( flashrom_write )
{
	logerror("Flash Write: %08X, %02X\n", offset, data);
}

static void irqhandler(int state)
{
	if (state)
	{
		cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, 0xd7);	// IRQ is RST10
	}
	else
	{
		cpunum_set_input_line(0, 0, CLEAR_LINE);
	}
}

static struct YMF271interface ymf271_interface =
{
	REGION_SOUND1,
	flashrom_read,
	flashrom_write,
	irqhandler
};

INPUT_PORTS_START( spimusic )
INPUT_PORTS_END

/********************************************************************************/

/* SPI */

static MACHINE_INIT( spimusic )
{
	UINT8 *sound = memory_region(REGION_SOUND1);

	UINT8 *rom = memory_region(REGION_USER2);
	UINT8 *prg = memory_region(REGION_USER1);
	UINT8 *sndprg = memory_region(REGION_CPU1);

	memory_set_bankptr(4, memory_region(REGION_CPU1));
	
	memcpy(sndprg, &prg[0x1bb800], 0x40000);
	
	memcpy(sound, &rom[0x200], 0x200000);
	
	z80_fifoin_push(0x82);
	z80_fifoin_push(0x57);
	z80_fifoin_push(0x8e);
	z80_fifoin_push(0x9e);
	z80_fifoin_push(0x82);
	z80_fifoin_push(0xbe);
	z80_fifoin_push(0x82);
	z80_fifoin_push(0xe6);
	z80_fifoin_push(0x81);
	z80_fifoin_push(0x49);
	z80_fifoin_push(0xe0);
	z80_fifoin_push(0x01);
	z80_fifoin_push(0x4a);
	z80_fifoin_push(0x4a);
	z80_fifoin_push(0x36);
	z80_fifoin_push(0xf8);
	z80_fifoin_push(0x08);
	z80_fifoin_push(0x93);
	z80_fifoin_push(0xff);
	z80_fifoin_push(0x83);
	z80_fifoin_push(0x01);
}

static MACHINE_DRIVER_START( spimusic )

	MDRV_CPU_ADD(Z80, 28636360/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(spisound_readmem, spisound_writemem)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(spimusic)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(6144)

	MDRV_VIDEO_START(spimusic)
	MDRV_VIDEO_UPDATE(spimusic)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMF271, 0)
	MDRV_SOUND_CONFIG(ymf271_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/*******************************************************************/

ROM_START(spimusic)
	ROM_REGION(0x40000, REGION_CPU1, 0)
	ROM_REGION32_LE(0x200000, REGION_USER1, 0)	/* i386 program */
	ROM_LOAD32_BYTE("gd_1.211", 0x000000, 0x80000, CRC(f6b2cbdc) SHA1(040c4ff961c8be388c8279b06b777d528c2acc1b) )
	ROM_LOAD32_BYTE("gd_2.212", 0x000001, 0x80000, CRC(1982f812) SHA1(4f12fc3fd7f7a4beda4d29cc81e3a58d255e441f) )
	ROM_LOAD32_BYTE("gd_3.210", 0x000002, 0x80000, CRC(b0f59f44) SHA1(d44fe074ddab35cd0190535cd9fbd7f9e49312a4) )
	ROM_LOAD32_BYTE("gd_4.29",  0x000003, 0x80000, CRC(cd8705bd) SHA1(b19a1486d6b899a134d7b518863ddc8f07967e8b) )

	ROM_REGION(0x200000, REGION_SOUND1, 0)

	ROM_REGION(0x200200, REGION_USER2, ROMREGION_ERASE00)	/* sound roms */
	ROM_LOAD("rdft.nv", 0x000000, 0x200200, CRC(c519a461) SHA1(e6d64eed0eedff636e7759fc72f3050b546a10d5) )
ROM_END

/* SPI */
GAME( 2005, spimusic,    0,       spimusic, spimusic, 0,   ROT0,   "MAME", "SPIMusic" )
