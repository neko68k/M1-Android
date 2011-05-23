/* Namco System 86 and it's immediate predecessor */

#include "m1snd.h"

#define M6809_CLOCK (49152000/32)

static void S86_Init(long srate);
static void S86_SendCmd(int cmda, int cmdb);
static void BDK_SendCmd(int cmda, int cmdb);

static unsigned int S86_Read(unsigned int address);
static void S86_Write(unsigned int address, unsigned int data);
static unsigned int S86_Readport(unsigned int port);
static void S86_Writeport(unsigned int port, unsigned int data);

static unsigned int TC2_Read(unsigned int address);
static void TC2_Write(unsigned int address, unsigned int data);
static unsigned int TC2_Readport(unsigned int port);
static void TC2_Writeport(unsigned int port, unsigned int data);

static unsigned int LOWROM, INPUT, UNK1, UNK2;

static int tc2_irqen = 0, tc2_stage = 4;

static unsigned char share_0[0x100];

/* signed/unsigned 8-bit conversion macros */
#define AUDIO_CONV(A) ((A)^0x80)

static int rt_totalsamples[7];
static int rt_decode_mode;


static int rt_decode_sample(void)
{
	struct GameSamples *samples;
	unsigned char *src, *scan, *dest, last=0;
	int size, n = 0, j;
	int decode_mode;

	Machine->samples = NULL;

	j = memory_region_length(REGION_SOUND1);
	if (j == 0) return 0;	/* no samples in this game */
	else if (j == 0x80000)	/* genpeitd */
		rt_decode_mode = 1;
	else
		rt_decode_mode = 0;

	logerror((char *)"pcm decode mode:%d\n", rt_decode_mode );
	if (rt_decode_mode != 0) {
		decode_mode = 6;
	} else {
		decode_mode = 4;
	}

	/* get amount of samples */
	for ( j = 0; j < decode_mode; j++ ) {
		src = memory_region(REGION_SOUND1)+ ( j * 0x10000 );
		rt_totalsamples[j] = ( ( src[0] << 8 ) + src[1] ) / 2;
		n += rt_totalsamples[j];
		logerror((char *)"rt_totalsamples[%d]:%d\n", j, rt_totalsamples[j] );
	}

	logerror((char *)"%d total samples\n", n);

	/* calculate the amount of headers needed */
	size = sizeof( struct GameSamples ) + n * sizeof( struct GameSamples * );

	/* allocate */
	if ( ( Machine->samples = (GameSamples *)malloc( size ) ) == NULL )
		return 1;

	samples = Machine->samples;
	samples->total = n;

	samples_init(n+1, Machine->sample_rate);

	for ( n = 0; n < samples->total; n++ ) {
		int indx, start, offs;

		if ( n < rt_totalsamples[0] ) {
			src = memory_region(REGION_SOUND1);
			indx = n;
		} else
			if ( ( n - rt_totalsamples[0] ) < rt_totalsamples[1] ) {
				src = memory_region(REGION_SOUND1)+0x10000;
				indx = n - rt_totalsamples[0];
			} else
				if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] ) ) < rt_totalsamples[2] ) {
					src = memory_region(REGION_SOUND1)+0x20000;
					indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] );
				} else
					if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] ) ) < rt_totalsamples[3] ) {
						src = memory_region(REGION_SOUND1)+0x30000;
						indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] );
					} else
						if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] ) ) < rt_totalsamples[4] ) {
							src = memory_region(REGION_SOUND1)+0x40000;
							indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] );
						} else
							if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] ) ) < rt_totalsamples[5] ) {
								src = memory_region(REGION_SOUND1)+0x50000;
								indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] );
							} else {
								src = memory_region(REGION_SOUND1)+0x60000;
								indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] + rt_totalsamples[5] );
							}

		/* calculate header offset */
		offs = indx * 2;

		/* get sample start offset */
		start = ( src[offs] << 8 ) + src[offs+1];

		/* calculate the sample size */
		scan = &src[start];
		size = 0;

		while ( *scan != 0xff ) {
			if ( *scan == 0x00 ) { /* run length encoded data start tag */
				/* get RLE size */
				size += scan[1] + 1;
				scan += 2;
			} else {
				size++;
				scan++;
			}
		}

		/* allocate sample */
		if ( ( samples->sample[n] = (GameSample *)malloc( sizeof( struct GameSample ) + size * 2 * sizeof( unsigned char ) ) ) == NULL )
			return 1;

		/* fill up the sample info */
		samples->sample[n]->length = size;
		samples->sample[n]->smpfreq = 6000;	/* 6 kHz */
		samples->sample[n]->resolution = 16;	/* 16 bit */

		/* unpack sample */
		dest = (unsigned char *)samples->sample[n]->data;
		scan = &src[start];

		while ( *scan != 0xff ) {
			if ( *scan == 0x00 ) { /* run length encoded data start tag */
				int i;
				for ( i = 0; i <= scan[1]; i++ ) /* unpack RLE */
				{
					*dest++ = 0;
					*dest++ = last;
				}

				scan += 2;
			} else {
				last = AUDIO_CONV( scan[0] );
				*dest++ = 0;
				*dest++ = last;
				scan++;
			}
		}

//		printf("sample %d: start %x length %d\n", n, (unsigned int)samples->sample[n]->data, samples->sample[n]->length);
		samples_set_info(n, 6000, samples->sample[n]->data, samples->sample[n]->length);
	}

	return 0; /* no errors */
}


/* play voice sample (Modified and Added by Takahiro Nogi. 1999/09/26) */
static int voice[2];

static void namco_voice_play( int offset, int data, int ch ) 
{
	if ( voice[ch] == -1 )
	{
	}
	else
	{
		samples_play_chan(voice[ch]);
	}
}
#if 0
static WRITE_HANDLER( namco_voice0_play_w ) {

	namco_voice_play(offset, data, 0);
}

static WRITE_HANDLER( namco_voice1_play_w ) {

	namco_voice_play(offset, data, 1);
}
#endif
/* select voice sample (Modified and Added by Takahiro Nogi. 1999/09/26) */
static void namco_voice_select( int offset, int data, int ch ) {

	logerror((char *)"Voice %d mode: %d select: %02x\n", ch, rt_decode_mode, data );

//	if ( data == 0 )
//		sample_stop( ch );

#if 0
	if (rt_decode_mode != 0) {
		switch ( data & 0xe0 ) {
			case 0x00:
			break;

			case 0x20:
				data &= 0x1f;
				data += rt_totalsamples[0];
			break;

			case 0x40:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1];
			break;

			case 0x60:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2];
			break;

			case 0x80:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3];
			break;

			case 0xa0:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4];
			break;

			case 0xc0:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] + rt_totalsamples[5];
			break;

			case 0xe0:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] + rt_totalsamples[5] + rt_totalsamples[6];
			break;
		}
	} else {
		switch ( data & 0xc0 ) {
			case 0x00:
			break;

			case 0x40:
				data &= 0x3f;
				data += rt_totalsamples[0];
			break;

			case 0x80:
				data &= 0x3f;
				data += rt_totalsamples[0] + rt_totalsamples[1];
			break;

			case 0xc0:
				data &= 0x3f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2];
			break;
		}
	}
#endif
	voice[ch] = data - 1;
}
#if 0
static WRITE_HANDLER( namco_voice0_select_w ) {

	namco_voice_select(offset, data, 0);
}

static WRITE_HANDLER( namco_voice1_select_w ) {

	namco_voice_select(offset, data, 1);
}
#endif
static struct YM2151interface ym2151_interface =
{
	1,          /* 1 chip */
	3579580,    /* 3.58 MHz */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	49152000/2048, 		/* 24000Hz */
	8,		/* number of voices */
	15,     /* playback volume */
	-1,		/* memory region */
	0		/* stereo */
};

static M16800T s1rwmem =
{
	S86_Read,
	S86_Write,
	S86_Readport,
	S86_Writeport,
};

static M16800T tc2rwmem =
{
	TC2_Read,
	TC2_Write,
	TC2_Readport,
	TC2_Writeport,
};

static void S86_Shutdown(void)
{
	struct GameSamples *samples;
	int i, total;

	samples = Machine->samples;

	if (!samples)
	{
		return;
	}

	total = samples->total;

	// free the samples
	for (i = 0; i < total; i++)
	{
		free(samples->sample[i]);
	}

	// free the samples struct
	free(Machine->samples);
	Machine->samples = NULL;
}

M1_BOARD_START( namcos86 )
	MDRV_NAME("Namco System 86")
	MDRV_HWDESC("HD63701, YM2151, Namco WSG")
	MDRV_INIT( S86_Init )
	MDRV_SEND( S86_SendCmd )
	MDRV_SHUTDOWN( S86_Shutdown )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(HD63701, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s1rwmem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

M1_BOARD_START( baraduke )
	MDRV_NAME("Baraduke")
	MDRV_HWDESC("HD63701, Namco WSG")
	MDRV_INIT( S86_Init )
	MDRV_SEND( BDK_SendCmd )
	MDRV_DELAYS( 600, 60 )

	MDRV_CPU_ADD(HD63701, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s1rwmem)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

static int ym2151_last_adr;

static unsigned int S86_Read(unsigned int address)
{
	if (address <= 0x1f)
	{
		return hd63701_internal_registers_r(address);
	}
	if (address >= 0x80 && address <= 0xff)
	{
		return workram[address];
	}

	if (address >= 0x1000 && address <= 0x10ff)
	{
		return namcos1_wavedata_r(address-0x1000);
	}

	if (address >= 0x1100 && address <= 0x113f)
	{
		return namcos1_sound_r(address-0x1100);
	}

	if (address >= 0x1140 && address <= 0x1fff)
	{
		return workram[address];
	}
	
	if (address >= INPUT && address <= INPUT+1)
	{
		return YM2151ReadStatus(0);
	}

	if (address == INPUT + 0x20)	// input 0
	{
		return 0;
	}	
	if (address == INPUT + 0x21)	// input 1
	{
		return 0;
	}	
	if (address == INPUT + 0x30)	// dip 0
	{
		return 0;
	}
	if (address == INPUT + 0x31)	// dip 1
	{
		return 0;
	}

	if (address >= LOWROM && address <= LOWROM+0x3fff)
	{
		return prgrom[address];
	}

	if (address >= 0x8000 && address <= 0xbfff)
	{
		return prgrom[address];
	}

	if (address >= 0xf000)
	{
		return prgrom[address];
	}

	return workram[address];
}

static void S86_Write(unsigned int address, unsigned int data)
{					
	if (address <= 0x1f)
	{
		hd63701_internal_registers_w(address, data);
		return;
	}

	if (address == INPUT)
	{
		ym2151_last_adr = data;
		return;
	}

	if (address == INPUT+1)
	{
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x1000 && address <= 0x10ff)
	{	
		namcos1_wavedata_w(address-0x1000, data);
		return;
	}

	if (address >= 0x1100 && address <= 0x113f)
	{	
		namcos1_sound_w(address-0x1100, data);
		return;
	}

	if (address >= 0x1140 && address <=0x1fff)
	{	
		workram[address] = data;
		return;
	}

	if (address == UNK1)
	{
//		printf("UNK1\n");
		return;	// hmm
	}

	if (address == UNK2)
	{
//		printf("UNK2\n");
		return;	// hmm
	}

	workram[address] = data;
//	printf("Unmapped write %x to %x\n", data, address);
}

static unsigned int S86_Readport(unsigned int port)
{
	switch (port)
	{
		case HD63701_PORT1:
			return 0;
			break;

		case HD63701_PORT2:
			return 0xff;
			break;
	}

	return 0;
}

static void S86_Writeport(unsigned int port, unsigned int data)
{
	switch (port)
	{
		case HD63701_PORT1:
//			printf("coin: %x\n", data);
			break;

		case HD63701_PORT2:
//			printf("LEDs: %x\n", data);
			break;
	}
}

static void S86_Init(long srate)
{
	namco_wavedata = &workram[0x1000];
	namco_soundregs = &workram[0x1100];

	LOWROM = INPUT = UNK1 = UNK2 = 0x20000;

	rt_decode_sample();

	switch (Machine->refcon)
	{
		case 1: //GAME_ROLLINGTHUNDER:
			LOWROM = 0x4000;
			INPUT = 0x2000;
			UNK1 = 0xb000;
			UNK2 = 0xb800;
			break;

		case 2: //GAME_WONDERMOMO:
			LOWROM = 0x4000;
			INPUT = 0x3800;
			UNK1 = 0xc000;
			UNK2 = 0xc800;
			break;

		case 3: //GAME_RETURNOFISHTAR:
			LOWROM = 0x0000;
			INPUT = 0x6000;
			UNK1 = 0x8000;
			UNK2 = 0x9800;
			break;

		case 4: //GAME_GENPEITOURMADEN:
			LOWROM = 0x4000;
			INPUT = 0x2800;
			UNK1 = 0xa000;
			UNK2 = 0xa800;
			break;

		case 5: //GAME_HOPPINGMAPPY:
		case 6: //GAME_SKYKIDDX:
			LOWROM = 0x4000;
			INPUT = 0x2000;
			UNK1 = 0x8000;
			UNK2 = 0x8800;
			break;

		case 7: //GAME_BARADUKE:
			m1snd_addToCmdQueue(0xbeef);
			m1snd_addToCmdQueue(0);
			m1snd_addToCmdQueue(0);
			m1snd_addToCmdQueue(0);
			break;
	}
}

static void S86_SendCmd(int cmda, int cmdb)
{
	int i;

	if ((cmda >= 0x200) && (cmda != 0xffff))
	{
		namco_voice_select(0, cmda-0x200+1, 0);
		namco_voice_play(0, 0, 0);
		return;
	}
	
	if (Machine->refcon == 5)	// hopping mappy
	{
		if (cmda == 0xffff)
		{
			for (i = 0; i < 32; i++)
			{
				if (workram[0x1285+i])
				{
					workram[0x1285+i]=0;
				}
			}
		}
		else
		{
			if (cmda < 32)
			{
				workram[0x1285+cmda] = 1;
			}
		}
	}
	else
	{
		if (Machine->refcon == 6)	// sky kid dx
		{
			if (cmda < 0x10)
			{	// BGM
				workram[0x1380] = cmda;
			}
			else	// SE
			{
				if (cmda >= 0x10 && cmda <= 0x110)
				{
					workram[0x1285+(cmda-0x10)] = 1;
				}
			}

			if (!cmda)
			{
				for (i = 0; i < 32; i++)
				{
					if (workram[0x1285+i])
					{
						workram[0x1285+i]=0;
					}
				}
			}
		}
		else
		{
			if (cmda < 0x100)
			{	// BGM
				if (Machine->refcon == 4) // avoid trashing in genpei
				{
					// commands over 51 in genpei kill the CPU
					if (cmda <= 51)
					{
						workram[0x1380] = cmda;
					}
				}
				else
				{
					workram[0x1380] = cmda;
				}
			}
			else	// SE
			{

				workram[0x1285+(cmda&0xff)] = 1;
			}

			if (!cmda)
			{
				for (i = 0; i < 64; i++)
				{
					if (workram[0x1285+i])
					{
						workram[0x1285+i]=0;
					}
				}
			}
		}
	}
}

static void BDK_SendCmd(int cmda, int cmdb)
{
	int i;

	if (cmda == 0xbeef)
	{
		workram[0x1183] = 1;
		return;
	}

	if (cmda == 0xffff)
	{
		{
			for (i = 0; i < 32; i++)
			{
				workram[0x1285+i] = 0;
				workram[0x1285+i+0x20] = 0;
			}
		}
/*
		{
			for (i = 0; i < 32; i++)
			{
				workram[0x1285+i] = 0;
				workram[0x1285+i+0x20] = 1;
			}
		}*/
		return;
	}

	workram[0x1285+cmda] = 1;
}

/* TC2 land */

static unsigned int ym_readmem(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom2[address];
	}

	if (address == 0x2001)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x3000 && address <= 0x30ff)
	{
		return share_0[(address & 0xff)];
	}

	return workram[0x10000 + address];
}

static void ym_writemem(unsigned int address, unsigned int data)
{
	if (address == 0x2000)
	{
//		printf("%x to 2151 adr\n", data);
		ym2151_last_adr = data;
		return;
	}

	if (address == 0x2001)
	{
//		printf("%x to 2151 @ %x\n", data, ym2151_last_adr);
		YM2151WriteReg(0, ym2151_last_adr, data);
		return;
	}

	if (address >= 0x3000 && address <= 0x30ff)
	{
//		if (data)
//		printf("6502: %x to shared @ %x\n", data, address);
		share_0[(address & 0xff)] = data;
	}
	
	workram[0x10000 + address] = data;
}

static M16502T tc2ymrw = 
{
	ym_readmem,
	ym_readmem,
	ym_writemem,
};

static unsigned int TC2_Read(unsigned int address)
{
	if (address >= 0x2000 && address <= 0x20ff) 
	{
//		printf("Rd from 6502 @ %x\n", address);
		return share_0[address & 0xff];
	}

	if (address <= 0x1f)
	{
		return hd63701_internal_registers_r(address);
	}

	if (address >= 0x80 && address <= 0xff)
	{
		return workram[address];
	}

	if (address >= 0x8000 && address <= 0xbfff)
	{
		return prgrom[address];
	}

	if (address >= 0xf000)
	{
		return prgrom[address];
	}

	if (address >= 0x1000 && address <= 0x10ff)
	{
		return namcos1_wavedata_r(address-0x1000);
	}

	if (address >= 0x1000 && address <= 0x1fff)
	{
		
		return workram[address];
	}
	
	return workram[address];
}

static void TC2_Write(unsigned int address, unsigned int data)
{					
	if (address >= 0x2000 && address <= 0x20ff) 
	{
		if (data)
			printf("%x to 6502 A @ %x\n", data, address);
		share_0[address&0xff] = data;
		return;
	}

	if (address <= 0x1f)
	{
		hd63701_internal_registers_w(address, data);
		return;
	}

	if (address >= 0x1000 && address <= 0x10ff)
	{	
		namcos1_wavedata_w(address-0x1000, data);
		return;
	}

	if (address >= 0x1100 && address <= 0x113f)
	{	
		namcos1_sound_w(address-0x1100, data);
		return;
	}

	if (address >= 0x1140 && address <=0x1fff)
	{	
		workram[address] = data;
		return;
	}

	if (address == 0x8000)
	{
		hd63701_set_irq_line(0, CLEAR_LINE);
		tc2_irqen = 0;
		return;
	}

	if (address == 0x8800)
	{
		tc2_irqen = 1;
		return;
	}

	workram[address] = data;
//	printf("Unmapped write %x to %x\n", data, address);
}

static unsigned int TC2_Readport(unsigned int port)
{
	switch (port)
	{
		case HD63701_PORT1:
			return 0xff;
			break;

		case HD63701_PORT2:
			return 0xff;
			break;
	}

	return 0;
}

static void TC2_Writeport(unsigned int port, unsigned int data)
{
	switch (port)
	{
		case HD63701_PORT1:
//			printf("coin: %x\n", data);
			break;

		case HD63701_PORT2:
//			printf("LEDs: %x\n", data);
			break;
	}
}

static void s1_timer(int refcon)
{
	if (tc2_irqen)
	{
		hd63701_set_irq_line(0, ASSERT_LINE);
	}

	// set up for next time
	timer_set(1.0/60.0, 0, s1_timer);
#if 0
	switch (tc2_stage)
	{
		case 0:
			tc2_stage++;
			break;
		case 1:
			workram[0x13a0] = 1;
			tc2_stage++;
			break;
		case 2:
			workram[0x13a0] = 0;
			tc2_stage++;
			break;
		default:
			break;
	}
#endif
	share_0[1] = 0x0e;
}

static void TC2_Init(long srate)
{
	namco_wavedata = &workram[0x1000];
	namco_soundregs = &workram[0x1100];

	LOWROM = INPUT = UNK1 = UNK2 = 0x20000;

	m1snd_addToCmdQueue(0xbeef);	// do boot protocol with MCU
	m1snd_addToCmdQueue(14);
	m1snd_addToCmdQueue(0xffff);

	tc2_irqen = 0;
	tc2_stage = 4;

	timer_set(1.0/60.0, 0, s1_timer);
}

static void TC2_SendCmd(int cmda, int cmdb)
{
	int i;

	if (cmda == 0xbeef)	// boot protocol
	{
		workram[0x1183] = 1;
		return;
	}

	if (cmda == 0xffff)
	{
		for (i = 0; i < 16; i++)
		{
  			workram[0x1286+i] = 0;
		}
		workram[0x13a0] = 0;
		return;
	}

	if (cmda < 16)
	{
		workram[0x1286+cmda] = 1;
	}
	else
	{
		workram[0x13a1] = cmda - 15;
		workram[0x13a0] = 0;
		tc2_stage = 0;
	}
}

M1_BOARD_START( tceptor2 )
	MDRV_NAME("Thunder Ceptor II")
	MDRV_HWDESC("HD63701, M65C02(x2), Namco WSG, YM2151, DAC")
	MDRV_INIT( TC2_Init )
	MDRV_SEND( TC2_SendCmd )
	MDRV_DELAYS( 3300, 180 )

	MDRV_CPU_ADD(HD63701, 49152000/32)
	MDRV_CPUMEMHAND(&tc2rwmem)

	MDRV_CPU_ADD(M65C02, 49152000/24)
	MDRV_CPUMEMHAND(&tc2ymrw)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END

