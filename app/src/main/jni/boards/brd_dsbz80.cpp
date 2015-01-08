/* 
	Sega Z80-based MPEG digital sound board 

	Star Wars "decrypt" by ElSemi.	
*/

#include "m1snd.h"
#include "mpeg.h"

static void DSB8_SendCmd(int cmda, int cmdb);

static int cmd_latch, chain = 0, mp_start, mp_end, mp_vol, mp_pan, mp_state, lp_start, lp_end;
static int status;

void cb(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	status &= ~2;

	if (chain == 0)
	{
		chain++;
		timer_set(1.0/240.0, 0, cb);
		status |= 2;			      
		return 0xaf;
	}

	if (chain == 1)
	{
		chain++;
		timer_set(1.0/240.0, 0, cb);
		status |= 2;
		return 0x6f;
	}

	return cmd_latch;
}

static READ_HANDLER( status_r )
{
	// bit 0 = ??? (must be 1 for most games)
	// bit 1 = command is pending (used by SWA instead of IRQ)
	// other bits = ???
	// SWA requires that status & 0x38 = 0 or else it loops endlessly...
	return status;
}

static WRITE_HANDLER( mpeg_trigger_w )
{
	char *ROM = (char *)memory_region(RGN_SAMP1);

	mp_state = data;

	if (data == 0)	// stop
	{
		MPEG_Stop_Playing();
		return;
	}

	if (data == 1)	// play without loop
	{
		MPEG_Set_Loop(NULL, 0);
		MPEG_Play_Memory(ROM + mp_start, mp_end-mp_start);
	}

	if (data == 2)	// play with loop
	{
		MPEG_Play_Memory(ROM + mp_start, mp_end-mp_start);
		return;
	}
}

static READ_HANDLER( mpeg_pos_r )
{
	int mp_prg = MPEG_Get_Progress();	// returns the byte offset currently playing

	mp_prg += mp_start;

	switch (offset)
	{
		case 0:
			return (mp_prg>>16)&0xff;
			break;
		case 1:
			return (mp_prg>>8)&0xff;
			break;
		case 2:
			return mp_prg&0xff;
			break;
	}

	return 0;
}

/* NOTE: writes to the start and end while playback is already in progress
   get latched.  When the current stream ends, the MPEG hardware starts playing
   immediately from the latched start and end position.  In this way, the Z80
   enforces looping where appropriate and multi-part songs in other cases
   (song #16 is a good example) 
*/

static WRITE_HANDLER( mpeg_start_w )
{
	static int start;
	char *ROM = (char *)memory_region(RGN_SAMP1);

	switch (offset)
	{
		case 0:
			start &= 0x00ffff;
			start |= (int)data<<16;
			break;
		case 1:
			start &= 0xff00ff;
			start |= (int)data<<8;
			break;
		case 2:
			start &= 0xffff00;
			start |= data;

			if (mp_state == 0)
			{
				mp_start = start;
			}
			else
			{
				lp_start = start;
				// SWA: if loop end is zero, it means "keep previous end marker"
				if (lp_end == 0)
				{
					MPEG_Set_Loop(ROM + lp_start, mp_end-lp_start);
				}
				else
				{
					MPEG_Set_Loop(ROM + lp_start, lp_end-lp_start);
				}
			}
			break;
	}
}

static WRITE_HANDLER( mpeg_end_w )
{
	static int end;
	char *ROM = (char *)memory_region(RGN_SAMP1);

	switch (offset)
	{
		case 0:
			end &= 0x00ffff;
			end |= (int)data<<16;
			break;
		case 1:
			end &= 0xff00ff;
			end |= (int)data<<8;
			break;
		case 2:
			end &= 0xffff00;
			end |= data;

			if (mp_state == 0)
			{
				mp_end = end;
			}
			else
			{
				lp_end = end;
				MPEG_Set_Loop(ROM + lp_start, lp_end-lp_start);
			}
			break;
	}
}

static void refresh_volpan(void)
{
	switch (mp_pan)
	{
		case 0:	// stereo
			mixer_set_stereo_volume(0, mp_vol*2, mp_vol*2);
			mixer_set_stereo_pan(0, MIXER_PAN_RIGHT, MIXER_PAN_LEFT);
			break;
		case 1:	// left only
			mixer_set_stereo_volume(0, 0, mp_vol*2);
			mixer_set_stereo_pan(0, MIXER_PAN_CENTER, MIXER_PAN_CENTER);
			break;
		case 2:	// right only
			mixer_set_stereo_volume(0, mp_vol*2, 0);
			mixer_set_stereo_pan(0, MIXER_PAN_CENTER, MIXER_PAN_CENTER);
			break;
	}
}

static WRITE_HANDLER( mpeg_volume_w )
{
	mp_vol = 0x7f - data;
	refresh_volpan();
}

static WRITE_HANDLER( mpeg_stereo_w )
{
	mp_pan = data & 3;
	refresh_volpan();
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0xe2, 0xe4, mpeg_pos_r },
	{ 0xf0, 0xf0, latch_r },
	{ 0xf1, 0xf1, status_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0xe0, 0xe0, mpeg_trigger_w },
	{ 0xe2, 0xe4, mpeg_start_w },
	{ 0xe5, 0xe7, mpeg_end_w },
	{ 0xe8, 0xe8, mpeg_volume_w },
	{ 0xe9, 0xe9, mpeg_stereo_w },
	{ 0xea, 0xea, IOWP_NOP },	// unknown, different values written each time, even for the same song
	{ 0xeb, 0xeb, IOWP_NOP },	// only written once on startup
	{ 0xf0, 0xf0, IOWP_NOP },	// command echoback
	{ 0xf1, 0xf1, IOWP_NOP },	// LEDs / status
PORT_END

static unsigned char data[0x400000];
static int n;
static unsigned short bitval=0;
static unsigned int bitcnt=0;

static int getbit(void)
{
	int b=bitval&0x8000;
	bitval<<=1;
	++bitcnt;
	if(bitcnt==8)
	{
		unsigned char c;
		c=data[n++];
		bitval|=c;
		bitcnt=0;
	}
	if(b)
		return 1;
	return 0;
}
static void DSB8_Init(long sr)
{
	unsigned char *rgn, c;

	// most DSB games (e.g. Scud Race) need bit 0 always set to run properly
	// SWA doesn't care, but it has other needs...
	status = 1;

	// "decrypt" SWA MPEG data
	if (Machine->refcon == 1)
	{
		memcpy(data, rom_getregion(REGION_SOUND1), 0x400000);

		c=data[n++];
		bitval=c<<8;
		c=data[n++];
		bitval|=c;
		bitcnt=0;

		getbit();
		getbit();
		getbit();

		rgn = rom_getregion(REGION_SOUND1);
		n = 0;

		while (n!=(0x400000-1))
		{
			unsigned char v=0;
			int i;
			for(i=0;i<8;++i)
			{
				int b=getbit();
				v|=b<<(7-i);
			}

			*rgn++ = v;
		}
	}
}

M1_BOARD_START( dsbz80 )
	MDRV_NAME("Sega Digital Sound Board (type 1)")
	MDRV_HWDESC("Z80, Sega 315-5762")
	MDRV_DELAYS(1000, 100)
	MDRV_INIT( DSB8_Init )
	MDRV_SEND( DSB8_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)	// the Z80 doesn't really do much here so it can be slow
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(MPEG, NULL)
M1_BOARD_END

static void DSB8_SendCmd(int cmda, int cmdb)
{
	chain = 0;
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	status |= 2;
}

// SWA songs: 2, 5, 11 (fanfare), 12 (cantina), 14, 15
