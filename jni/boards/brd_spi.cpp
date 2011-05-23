/*

	Seibu SPI driver
	Big thanks to Ville Linde.
*/

#include "m1snd.h"

#define FIFO_SIZE 512
static int fifoin_rpos, fifoin_wpos;
static UINT8 fifoin_data[FIFO_SIZE];
static int fifoin_read_request = 0;

static int fifoout_rpos, fifoout_wpos;
static UINT8 fifoout_data[FIFO_SIZE];
static int fifoout_read_request = 0;

#if 0
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
#endif

static void z80_fifoout_push(UINT8 data)
{
	fifoout_data[fifoout_wpos++] = data;
	if (fifoout_wpos == FIFO_SIZE)
	{
		fifoout_wpos = 0;
	}
	if(fifoout_wpos == fifoout_rpos)
	{
		printf("Sound FIFOOUT overflow at %08X\n", activecpu_get_pc());
	}

	fifoout_read_request = 1;
}

static UINT8 z80_fifoin_pop(void)
{
	UINT8 r;
	if (fifoin_wpos == fifoin_rpos)
	{
		printf("Sound FIFOIN underflow at %08X\n", activecpu_get_pc());
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
		printf("Sound FIFOIN overflow at %08X\n", activecpu_get_pc());
	}

	fifoin_read_request = 1;
}

static int lastbank = -1;

static WRITE_HANDLER( bank_w )
{
	int bank;

	bank = data & 7;
	bank *= 0x8000;

	if (bank != lastbank)
	{
//		printf("z80 bank to %x\n", bank);

		cpu_setbank(1, memory_region(REGION_CPU1) + bank);

		lastbank = bank;
	}
}

static void SPI_Init(long srate)
{
	unsigned char *ROM = memory_region(RGN_CPU2);
	unsigned char *RAM = workram;
	int offset;
	UINT8 *sound = memory_region(REGION_SOUND1);
	UINT8 *rom = memory_region(REGION_USER2);

/*
rdft/rdftu/rdftau   0x1bb800
rdftj               0x19cf00
viprp1/viprp1s      0x1a5200
viprp1o             0x1a0e00
viprp1ot            0x19fe00
senkyu/batlball/batlbala    0xd6c00
ejanhs              0xb4100

The Z80 program is in the sound1 rom in RF2 and RFJet
rfjet               0x44000 (sound1.u222)
rdft2               0x60000 (rf2_8_sound1.bin)
*/

	switch (Machine->refcon)
	{
		case 1:
			offset = 0x1a5200;	// viprp1/viprp1s
			break;

		case 2:
			offset = 0x19cf00;	// rdftj
			break;

		case 3:
			offset = 0x1a0e00;	// viprp1o
			break;

		case 4:
			offset = 0x19fe00;	// viprp1ot
			break;

		case 5:
			offset = 0xd6c00;	// senkyu/batlball/batlbala
			break;

		case 6:
			offset = 0xb4100;	// ejanhs
			break;

		case 7:
			offset = 0x44000;	// rfjet
			break;

		case 8:
			offset = 0x60000;	// rf2
			break;

		default:
			offset = 0x1bb800;	// rdft/rdftu/rdftau
			break;
	}

	// extract the Z80 code+data from the i386 ROMs
	memcpy(RAM, ROM+offset, 0x40000);
	memcpy(memory_region(REGION_CPU1), ROM+offset, 0x40000);

	// extract the sample data from the MAME nvram save
	memcpy(sound, &rom[0x200], 0x200000);

	bank_w(0, 0);

	// push startup negotiation
	// checksum first
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

	if (Machine->refcon == 5)
	{
		z80_fifoin_push(0xe0);				 
		z80_fifoin_push(0x01);	// country code		 
		z80_fifoin_push(0x50);				 
		z80_fifoin_push(0x5a);				 
		z80_fifoin_push(0x31);	// game number		 
		z80_fifoin_push(0xf8);				 
		z80_fifoin_push(0x08);				 
		z80_fifoin_push(0x93);
		z80_fifoin_push(0xff);
		z80_fifoin_push(0x93);
		z80_fifoin_push(0xff);
		z80_fifoin_push(0x80);
		z80_fifoin_push(0x05);
		z80_fifoin_push(0x80);
		z80_fifoin_push(0x54);
	}
	else
	{							 
		z80_fifoin_push(0xe0);				 
		z80_fifoin_push(0x01);	// country code		 
		z80_fifoin_push(0x4a);				 
		z80_fifoin_push(0x4a);				 
		z80_fifoin_push(0x36);	// game number		 
		z80_fifoin_push(0xf8);				 
		z80_fifoin_push(0x08);				 
		z80_fifoin_push(0x93);
		z80_fifoin_push(0xff);
	}
}							 

static void SPI_SendCmd(int cmda, int cmdb)
{
	if (!cmda) return;

	z80_fifoin_push(0x93);
	z80_fifoin_push(0xff);
	z80_fifoin_push((0x80) & 0xff);
	z80_fifoin_push(cmda & 0xff);
}

static READ8_HANDLER( z80_soundfifo_r )
{
	UINT8 r = z80_fifoin_pop();

//	printf("Rd %x from fifo\n", r);

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

static READ_HANDLER( ram_r )
{
	return workram[offset];
}

static WRITE_HANDLER( ram_w )
{
	workram[offset] = data;
}

static WRITE_HANDLER( unk_w )
{
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x3fff, ram_r },
	{ 0x4008, 0x4008, z80_soundfifo_r },
	{ 0x4009, 0x4009, z80_soundfifo_status_r },
	{ 0x400a, 0x400a, z80_jp1_r },
	{ 0x4013, 0x4013, z80_coin_r },
	{ 0x6000, 0x600f, YMF271_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x3fff, ram_w },
	{ 0x4002, 0x4002, unk_w },	// ack RST 10
	{ 0x4003, 0x4003, unk_w },	// unknown
	{ 0x4008, 0x4008, z80_soundfifo_w },
	{ 0x400b, 0x400b, unk_w },	// unknown
	{ 0x401b, 0x401b, bank_w },
	{ 0x6000, 0x600f, YMF271_0_w },
MEMORY_END

static PORT_READ_START( snd_readport )
PORT_END

static PORT_WRITE_START( snd_writeport )
PORT_END

static READ8_HANDLER( flashrom_read )
{
	UINT8 *flashrom = memory_region(REGION_SOUND1);
//	printf("Flash Read: %08X\n", offset);
	
	return flashrom[offset & 0x1fffff];
}

static WRITE8_HANDLER( flashrom_write )
{
	logerror((char *)"Flash Write: %08X, %02X\n", offset, data);
}

extern "C" { void z80_set_irqvec(int vector); }

static void opx_irq(int state)
{
//	printf("opx_irq: %d\n", state);
	if (state)
	{
		z80_set_irqvec(Z80_RST_10);
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	}
	else
	{
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

static struct YMF271interface ymf271_interface =
{
	1,
	flashrom_read,
	flashrom_write,
	{ REGION_SOUND1, },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT),},
	{ opx_irq },
};

M1_BOARD_START( spi )
	MDRV_NAME( "Seibu SPI" )  
	MDRV_HWDESC( "Z80, YMF271 (OPX)")
	MDRV_DELAYS( 2000, 100 )
	MDRV_INIT( SPI_Init )
	MDRV_SEND( SPI_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YMF271, &ymf271_interface)
M1_BOARD_END

