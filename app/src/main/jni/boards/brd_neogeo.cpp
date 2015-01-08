// Neo Geo (Z80, YM2610)

#include "m1snd.h"

#define Z80_CLOCK (6000000)
#define YM_CLOCK (8000000)

static void YM_2610IRQHandler(int irq);

static unsigned int z80_in_neo(unsigned int address);
static void z80_out_neo(unsigned int address, unsigned int data);

static int nbnk8, nbnkc, nbnke, nbnkf;

static struct YM2610interface neogeo_ym2610_interface =
{
	1,
	8000000,
	{ MIXERG(30,MIXER_GAIN_1x,MIXER_PAN_CENTER) },	// PSG volume
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_2610IRQHandler },
	{ RGN_SAMP2 },
	{ RGN_SAMP1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }	// FM volume
};

static READ_HANDLER( neo_readport2 )
{
	return z80_in_neo(offset);
}

static WRITE_HANDLER( neo_writeport2 )
{
	z80_out_neo(offset, data);
}

static MEMORY_READ_START( neo_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_BANK2 },
	{ 0xe000, 0xefff, MRA_BANK3 },
	{ 0xf000, 0xf7ff, MRA_BANK4 },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( neo_writemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( neo_readport )
	{ 0x0000, 0xffff, neo_readport2 },
PORT_END

static PORT_WRITE_START( neo_writeport )
	{ 0x0000, 0xffff, neo_writeport2 },
PORT_END
static unsigned char cmd_latch;

static unsigned int z80_in_neo(unsigned int port)
{
	int porth = (port>>8);

	port &= 0xff;
	switch (port)
	{
		case 0x0:
//			printf("Reading command %x\n", cmd_latch);
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
			return cmd_latch;
			break;
		case 0x4:
			return YM2610Read(0, 0);
			break;
		case 0x5:
			return YM2610Read(0, 1);
			break;
		case 0x6:
			return YM2610Read(0, 2);
			break;
		case 0x8:	// 2k window at f000
			nbnkf = (porth&0x7f)*0x800;
			cpu_setbank(4, &prgrom[nbnkf]);
//			printf("2k bank: porth = %x res = %x\n", porth, nbnkf);
			return 0;
			break;
		case 0x9:	// 4k window at e000
			nbnke = (porth&0x3f)*0x1000;
			cpu_setbank(3, &prgrom[nbnke]);
//			printf("4k bank: porth = %x res = %x\n", porth, nbnke);
			return 0;
			break;	
		case 0xa:	// 8k window at c000
			nbnkc = (porth&0x1f)*0x2000;
			cpu_setbank(2, &prgrom[nbnkc]);
//			printf("8k bank: porth = %x res = %x\n", porth, nbnkc);
			return 0;
			break;
		case 0xb:	// 16k window at 8000
			nbnk8 = (porth&0xf)*0x4000;
			cpu_setbank(1, &prgrom[nbnk8]);
//			printf("16k bank: porth = %x res = %x\n", porth, nbnk8);
			return 0;
			break;
	}

//	printf("Unknown read from port %x (PC=%x)\n", port, _z80_get_reg(Z80_REG_PC));
	return 0;
}

static void z80_out_neo(unsigned int port, unsigned int val)
{
	port &= 0xff;
	switch (port)
	{
		case 0x04:
			YM2610Write(0, 0, val);
			break;
		case 0x05:
			YM2610Write(0, 1, val);
			break;
		case 0x06:
			YM2610Write(0, 2, val);
			break;
		case 0x07:
			YM2610Write(0, 3, val);
			break;
		case 0x0c:
		case 0xc0:
			break;
		default:
//			printf("Unknown write %x to port %x\n", val, port);
			break;
	}
}

static void YM_2610IRQHandler(int irq)
{
//	printf("YM2610 IRQ! irq=%d\n", irq);

	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void neo_pcm2_swap(int value) /* 0=kof2002, 1=matrim */
{
	static const unsigned int addrs[7][2]={
		{0x000000,0xA5000},
		{0xFFCE20,0x01000},
		{0xFE2CF6,0x4E001},
		{0xFFAC28,0xC2000},
		{0xFEB2C0,0x0A000},
		{0xFF14EA,0xA7001},
		{0xFFB440,0x02000}};
	static const UINT8 xordata[7][8]={
		{0xF9,0xE0,0x5D,0xF3,0xEA,0x92,0xBE,0xEF},
		{0xC4,0x83,0xA8,0x5F,0x21,0x27,0x64,0xAF},
		{0xC3,0xFD,0x81,0xAC,0x6D,0xE7,0xBF,0x9E},
		{0xC3,0xFD,0x81,0xAC,0x6D,0xE7,0xBF,0x9E},
		{0xCB,0x29,0x7D,0x43,0xD2,0x3A,0xC2,0xB4},
		{0x4B,0xA4,0x63,0x46,0xF0,0x91,0xEA,0x62},
		{0x4B,0xA4,0x63,0x46,0xF0,0x91,0xEA,0x62}};
	UINT8 *src = (UINT8 *)memory_region(REGION_SOUND1);
	UINT8 *buf = (UINT8 *)malloc(0x1000000);
	int i, j, d;

	memcpy(buf,src,0x1000000);
	for (i=0;i<0x1000000;i++)
	{
		j=BITSWAP24(i,23,22,21,20,19,18,17,0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,16);
		j=j^addrs[value][1];
		d=((i+addrs[value][0])&0xffffff);
		src[j]=buf[d]^xordata[value][j&0x7];
	}
	free(buf);
}

static void neo_pcm2_snk_1999(int value) /* 8=mslug4, 16=rotd */
{	/* thanks to Elsemi for the NEO-PCM2 info */
	data16_t *rom = (data16_t *)memory_region(REGION_SOUND1);
	int size = memory_region_length(REGION_SOUND1);
	int i, j;

	if( rom != NULL )
	{	/* swap address lines on the whole ROMs */
		data16_t *buffer = (data16_t *)malloc((value / 2) * sizeof(data16_t));
		if (!buffer)
			return;

		for( i = 0; i < size / 2; i += ( value / 2 ) )
		{
			memcpy( buffer, &rom[ i ], value );
			for( j = 0; j < (value / 2); j++ )
			{
				rom[ i + j ] = buffer[ j ^ (value/4) ];
			}
		}
		free(buffer);
	}
}

static void Neo_Init(long srate)
{
	// if a delta-t region doesn't exist, map both to the same rom
	if (!rom_getregionsize(RGN_SAMP2))
	{
		// horrible cheese
		neogeo_ym2610_interface.pcmromb[0] = RGN_SAMP1;
	}
	else
	{
		neogeo_ym2610_interface.pcmromb[0] = RGN_SAMP2;
	}

	// init bank offsets
	nbnk8 = 0x8000;
	nbnkc = 0xc000;
	nbnke = 0xe000;
	nbnkf = 0xf000;

	cpu_setbank(4, &prgrom[nbnkf]);
	cpu_setbank(3, &prgrom[nbnke]);
	cpu_setbank(2, &prgrom[nbnkc]);
	cpu_setbank(1, &prgrom[nbnk8]);

	// super sidekicks4's dump is slightly out of order: the first 2 megs goes
	// at 0x200000 and the second at 000000.  rearrange here.
	if (Machine->refcon == 7)
	{
		char *s1, *s2, *d;

		s1 = s2 = d = (char *)rom_getregion(RGN_SAMP1);

		s2 = s1 + 0x200000;
		d = s1 + 0x600000;
		memcpy(d, s2, 0x200000);
		memcpy(s2, s1, 0x200000);
		memcpy(s1, d, 0x200000);
	}

	if (Machine->refcon == 6)   	// rotd decrypt
	{
	}

	if (Machine->refcon == 8)	// kof2002 decrypt
	{
	}

	if (Machine->refcon == 9)	// matrimelee decrypt
	{
	}

	if (Machine->refcon == 10)	// mslug4 decrypt
	{
	}

	// set up decrypt
	switch (Machine->refcon)
	{
		case 6:	// rotd
			neo_pcm2_snk_1999(16);
			break;

		case 8:	// kof2002
			neo_pcm2_swap(0);
			break;

		case 9:	// matrimelee
			neo_pcm2_swap(1);
			break;

		case 10:	// mslug4
			neo_pcm2_snk_1999(8);
			break;

		case 11:	// mslug5
			neo_pcm2_swap(2);
			break;
		
		case 12:	// svc
			neo_pcm2_swap(3);
			break;

		case 13:	// samsho5
			neo_pcm2_swap(4);
			break;

		case 14:	// kof2003
			neo_pcm2_swap(5);
			break;

		case 15:	// samsh5sp
			neo_pcm2_swap(6);
			break;

		case 16:	// pnyaa
			neo_pcm2_snk_1999(4);
			break;
	}

	// set up command format
	switch (Machine->refcon)
	{
		// most Alpha Denshi games and a few others (the metal slugs) have
		// no command prefix for music.
		case 1:
			break;

		// most Data East (DECO) games need special treatment
		case 2:
			m1snd_addToCmdQueue(0x16);
			m1snd_addToCmdQueue(0x16);
			m1snd_addToCmdQueue(0x19);
			m1snd_addToCmdQueue(0x16);
			m1snd_addToCmdQueue(0x16);
			m1snd_setNoPrefixOnStop();
			break;

		// some Alpha Denshi games are different in a different way
		// (just to be different no doubt)
		case 3:
			m1snd_setCmdPrefix(0xfc);
			break;

		// WH2Jet and WHPerfect have yet another prefix
		case 4:
			m1snd_setCmdPrefix(0xfd);
			break;

		// Puzzle Bobble 2 needs a very special init sequence to work properly
		case 5:
			m1snd_addToCmdQueue(1);
			m1snd_addToCmdQueue(3);
			m1snd_addToCmdQueue(3);
			m1snd_addToCmdQueue(1);
			m1snd_addToCmdQueue(3);
			m1snd_addToCmdQueue(7);
			m1snd_addToCmdQueue(0x19);
			m1snd_addToCmdQueue(0x1a);
			m1snd_addToCmdQueue(0x19);
			break;

		// most games use SNK's default sound driver and are happy that way...
		case 7:	// neo bomberman and super sidekicks 4 need some special attention but still use the default
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		default:
			m1snd_setCmdPrefix(7);
			break;
	}
}

static void Neo_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void Neo_Shutdown(void)
{
	YM2610Shutdown();
}

M1_BOARD_START( neogeo )
	MDRV_NAME( "Neo Geo" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( Neo_Init )
	MDRV_SEND( Neo_SendCmd )
	MDRV_SHUTDOWN( Neo_Shutdown )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(neo_readmem, neo_writemem)
	MDRV_CPU_PORTS(neo_readport, neo_writeport)

	MDRV_SOUND_ADD(YM2610, &neogeo_ym2610_interface)
M1_BOARD_END

