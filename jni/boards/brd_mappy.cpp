/* Namco Mappy and related hardware */

#include "m1snd.h"

#define M6809_CLOCK (18432000/12)
#define YM_CLOCK (3579580)

#define Map_TIMER_PERIOD (1.0/60.0)

static void Map_Init(long srate);
static void Map_SendCmd(int cmda, int cmdb);

extern unsigned char *mappy_soundregs;
WRITE_HANDLER( mappy_sound_w );
WRITE_HANDLER( mappy_sound_enable_w );

static unsigned int Map_Read(unsigned int address);
static void Map_Write(unsigned int address, unsigned int data);

static int irqmask = 0, hseq;
static unsigned int IRQON, IRQOFF, SNDON, SNDOFF;

static struct namco_interface namco_interface =
{
	24000,	/* sample rate */
	8,		/* number of voices */
	100,	/* playback volume */
	RGN_SAMP1	/* memory region */
};

static struct DACinterface dac_interface =
{
	1,
	{ 55 }
};

static M16809T s1rwmem =
{
	Map_Read,
	Map_Write,
};

M1_BOARD_START( mappy )
	MDRV_NAME("Mappy")
	MDRV_HWDESC("M6809, Namco WSG, DAC")
	MDRV_INIT( Map_Init )
	MDRV_SEND( Map_SendCmd )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&s1rwmem)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static WRITE_HANDLER( grobda_DAC_w )
{
	DAC_data_w(0, (data << 4) | data);
}

static unsigned int Map_Read(unsigned int address)
{
	if (address <= 0x3f)
	{
		return workram[address];
	}

	if (address >= 0x40 && address <= 0x3ff)
	{
		if (hseq != -1)
		{
			switch (Machine->refcon)
			{
				case 1: //GAME_MOTOS:
				case 6: //GAME_GROBDA:
					if (address == 0x40)
					{
						switch(hseq)
						{
							case 1:
								workram[0x40] = 0x43;
								workram[0x41] = 0x4b;
								break;

							case 0:
								workram[0x40] = 0x47;
								workram[0x41] = 0x4f;
								break;
						}
						hseq--;
					}
					break;

				case 2: //GAME_GAPLUS:
					if(address == 0x40)
					{
						workram[0x40] = 0x11;
						hseq=-1;
					}
					break;

				case 3: //GAME_TOYPOP:
					if(address == 0x3fc)
					{
						hseq=-1;
						irqmask=1;
					}
					break;

				case 4: //GAME_SUPERPACMAN:
					if(address == 0x40)
					{
						workram[0x40] = 0;
						workram[0x41] = 0;
						hseq=-1;
						irqmask=1;
					}
					break;
				case 5: //GAME_PHOZON:
					if (address == 0x41)
					{
						workram[0x41] = 0;
						irqmask=1;
					}
					break;
			}
		}

		return workram[address];
	}

	if (address >= 0xe000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void Map_Write(unsigned int address, unsigned int data)
{		
	if (address == 0x0002 && Machine->refcon == 6)
	{
		grobda_DAC_w(0, data);
	}

	if (address <= 0x3f)
	{
		mappy_sound_w(address, data);
		return;
	}
			
	if (address >= 0x40 && address <= 0x3ff)
	{
		workram[address] = data;
		return;
	}

	if (address == IRQOFF)
	{
//		printf("IRQs off\n");
		irqmask = 0;
		return;
	}

	if (address == IRQON)
	{
//		printf("IRQs on\n");
		irqmask = 1;
		return;
	}

	if (address >= SNDOFF && address <= SNDON)
	{
		mappy_sound_enable_w(address-0x2006, data);
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void map_timer(int refcon)
{
	if (irqmask)
	{
		if (Machine->refcon == 4)
		{
			workram[0xfb]++;
		}
		else
		{
			cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
			cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);	
		}
	}

	// set up for next time
	timer_set(Map_TIMER_PERIOD, 0, map_timer);
}

static void Map_Init(long srate)
{
	mappy_soundregs = &workram[0];

	timer_set(Map_TIMER_PERIOD, 0, map_timer);

	IRQOFF = 0x2000;
	IRQON = 0x2001;
	SNDOFF = 0x2006;
	SNDON = 0x2007;

	switch (Machine->refcon)
	{
		case 3: //GAME_TOYPOP:
			SNDOFF = 0x2000;
			SNDON = 0x2001;

		// (intentional fallthrough)
		case 2: //GAME_GAPLUS:
		case 7: //GAME_LIBBLERABBLE:
			IRQOFF = 0x6000;
			IRQON = 0x4000;
			hseq = 0;
			break;

		case 4: //GAME_SUPERPACMAN:
		case 5: //GAME_PHOZON:
			SNDOFF = 0xffff;
			SNDON = 0xffff;
			hseq = 0;
			break;

		case 1: //GAME_MOTOS:
		case 6: //GAME_GROBDA:
			hseq = 1;
			break;

		default:
			hseq = -1;
			break;
	}
}

static void Map_SendCmd(int cmda, int cmdb)
{
	int i, base;

	base = 0x40;
	if (Machine->refcon == 5)
	{
		base = 0x50;
	}

	if (cmda != 0xffff)
	{
		if (cmda >= 0x20)
		{
			return;
		}
		
		if (Machine->refcon == 6)
		{
			if (cmda != 2)
			{
				workram[0x40 + cmda] = 1;
			}
		}
		else if (Machine->refcon == 5)
		{
			workram[0x50 + cmda] = 1;
		}
		else
		{
			workram[0x40 + cmda] = 1;
		}
	}
	else
	{
		if ((Machine->refcon == 4) || (Machine->refcon == 9) || (Machine->refcon == 5))
		{
			for(i = 0; i < 32; i++)
			{
				if(workram[base+i])
				{
					workram[base+i]=0;
					workram[base+0x20+i]=0;
				}
			}
		}
		else
		{
			for(i = 0; i < 32; i++)
			{
				if(workram[base+i])
				{
					workram[base+i]=0;
					workram[base+0x20+i]=1;
				}
			}
		}
	}
}
