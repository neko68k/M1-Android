/* mame-compatible stream/mixer system */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "driver.h"

#define SFL_ALLOCED	(1)	// stream has been set up
#define SFL_RESAMPLE	(2)	// stream needs resampling before mix

extern int nDSoundSegLen;

typedef struct
{
	int flags;
	int numchan;
	int srate;
	int param;
	void (*callbacks)(int param, INT16 *buffer, int length);
	void (*callbackm)(int param, INT16 **buffer, int length);
	long curpos;
	INT16 *ichannels[8];	// raw data from the source
	INT16 *ochannels[8];	// resampled data ready to mix
	INT16 chanvol[8];
	int chanpan[8];
	char channame[8][128];
} StreamChanT;

static StreamChanT stream[MAX_STREAMS];

static int numstreams;
static int bSoundEnable = 1;

static INT16 *chanptrs[8];

void set_RC_filter(int channel,int R1,int R2,int R3,int C)
{
}

int streams_sh_start(void)
{
	numstreams = 0;
	bSoundEnable = 1;
	return 0;
}

void streams_sh_stop(void)
{
	int i, j;

	for (i = 0; i < numstreams; i++)
	{
		for (j = 0; j < stream[i].numchan; j++)
		{
			free(stream[i].ichannels[j]);
			free(stream[i].ochannels[j]);
		}

		stream[i].flags = 0;
		stream[i].numchan = 0;
		stream[i].srate = 0;
	}

	numstreams = 0;
}

void streams_sh_update(void)
{
}

int stream_init(const char *name,int default_mixing_level,
		int sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length))
{
	stream[numstreams].flags = SFL_ALLOCED;
	stream[numstreams].numchan = 1;
	stream[numstreams].srate = sample_rate;
	stream[numstreams].param = param;
	stream[numstreams].callbacks = callback;
	stream[numstreams].curpos = 0;
	stream[numstreams].ichannels[0] = (INT16 *)malloc(sample_rate * 2 * 2);	
	stream[numstreams].ochannels[0] = (INT16 *)malloc(nDSoundSegLen * 2 * 2);	
	stream[numstreams].chanvol[0] = (MIXER_GET_LEVEL(default_mixing_level) * 255) / 100;
	stream[numstreams].chanpan[0] = MIXER_PAN_CENTER; //MIXER_GET_PAN(default_mixing_level);
	strcpy(stream[numstreams].channame[0], name);

//	printf("st %d [%s] = %d rt %d pan %d (%d)\n", numstreams, name, stream[numstreams].chanvol[0], sample_rate, stream[numstreams].chanpan[0], stream[numstreams].param);

	if (sample_rate != Machine->sample_rate)
	{
		stream[numstreams].flags |= SFL_RESAMPLE;
	}

	numstreams++;

	return 0;
}

int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		int sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length))
{
	int i;

	stream[numstreams].flags = SFL_ALLOCED;
	stream[numstreams].numchan = channels;
	stream[numstreams].srate = sample_rate;
	stream[numstreams].param = param;
	stream[numstreams].callbackm = callback;
	stream[numstreams].curpos = 0;

	if (sample_rate != Machine->sample_rate)
	{
		stream[numstreams].flags |= SFL_RESAMPLE;
	}

	for (i = 0; i < channels; i++)
	{
		stream[numstreams].ichannels[i] = (INT16 *)malloc(sample_rate * 2 * 2);	
		stream[numstreams].ochannels[i] = (INT16 *)malloc(nDSoundSegLen * 2 * 2);	
		stream[numstreams].chanvol[i] = (MIXER_GET_LEVEL(default_mixing_levels[i]) * 255) / 100;
		stream[numstreams].chanpan[i] = MIXER_GET_PAN(default_mixing_levels[i]);
		strcpy(stream[numstreams].channame[i], names[i]);

//		printf("st %d ch %d [%s] = %d pan %d rt = %d\n", numstreams, i, names[i], stream[numstreams].chanvol[i], stream[numstreams].chanpan[i], sample_rate);
	}

	numstreams++;

	return (numstreams-1);
}

void stream_set_srate(int streamnum, int sample_rate)
{
	stream[streamnum].srate = sample_rate;

	if (sample_rate != Machine->sample_rate)
	{
		stream[streamnum].flags |= SFL_RESAMPLE;
	}
}

void stream_update(int channel,int min_interval)	/* min_interval is in usec */
{
	if (!min_interval)	// if zero, force a hard sync of all CPUs and DACs
	{
		if (timer_get_cur_dsp_time() >= 1.0)
		{
			timer_yield();
		}
	}
	else
	{
//		logerror("stream_update with nonzero min_interval\n");

		if (timer_get_cur_dsp_time() >= min_interval)
		{
			timer_yield();
		}
	}
}

void StreamSys_Update(long dsppos, long dspframes)
{
	int i, j;
	int target;

	if (dsppos < 0 || dsppos > Machine->sample_rate/30) return;
	if (dspframes <= 0) return;

//	printf("%d streams\n", numstreams);

	for (i = 0; i < numstreams; i++)
	{
		// get position in the unresampled buffer
		target = ((dsppos + dspframes) * stream[i].srate) / Machine->sample_rate;
		
		if (stream[i].curpos < target)
		{
			if (stream[i].numchan == 1)
			{
				// mono stream
				stream[i].callbacks(stream[i].param, &stream[i].ichannels[0][stream[i].curpos], target - stream[i].curpos);
				stream[i].curpos = target;
			}
			else
			{
				for (j = 0; j < stream[i].numchan; j++)
				{
					chanptrs[j] = &stream[i].ichannels[j][stream[i].curpos];
					if (!chanptrs[j]) return;
				}

				stream[i].callbackm(stream[i].param, chanptrs, target - stream[i].curpos);
				stream[i].curpos = target;
			}
		}
	}	
}

void StreamSys_Run(signed short *out, long samples)
{
	int i, j, k;
	INT64 accL, accR;
//	FILE *f;
//	signed short *outcopy = out;

	// make sure all DSPs are up to date
	StreamSys_Update(0, samples);

	if (!bSoundEnable)
	{
		memset(out, 0, samples*4);
		return;
	}

	// phase 1: remix streams needing it
	for (i = 0; i < numstreams; i++)
	{
		if (stream[i].srate != Machine->sample_rate)
		{
			unsigned int sfrac, sint, sfracptr, sintptr, sacc;
			double intr;
			INT16 *src, *dst;

			intr = ((double)stream[i].srate / (double)Machine->sample_rate) * 65536.0;
			sint = ((int) intr)>>16;
			sfrac = ((int) intr) & 0xffff;

//			printf("resampling from %d to %ld (int = %x, frac = %x, samples = %ld, chan = %d)\n", stream[i].srate, Machine->sample_rate, sint, sfrac, samples, stream[i].numchan);

			for (j = 0; j < stream[i].numchan; j++)
			{
				sfracptr = sintptr = sacc = 0;
				src = (INT16 *)&stream[i].ichannels[j][0];
				dst = (INT16 *)&stream[i].ochannels[j][0];

				for (k = 0; k < samples; k++)
				{
					if (sacc >= stream[i].curpos)
					{
						sacc = (stream[i].curpos-1);
					}
					*dst++ = src[sacc];

					sfracptr += sfrac;
					sacc += sint;

					if (sfracptr > 0xffff)
					{
						sacc += (sfracptr>>16);
						sfracptr &= 0xffff;
					}
				}
			}
		}
		else
		{
			for (j = 0; j < stream[i].numchan; j++)
			{
				memcpy(&stream[i].ochannels[j][0], &stream[i].ichannels[j][0], samples*2);
			}
		}
	}

	// phase 2: mix
	for (j = 0; j < samples; j++)
	{
		accL = accR = 0;
		for (i = 0; i < numstreams; i++)
		{
			for (k = 0; k < stream[i].numchan; k++)
			{
				switch (stream[i].chanpan[k])
				{
					case MIXER_PAN_CENTER:
						accL += (stream[i].ochannels[k][j] * stream[i].chanvol[k]) >> 8;
						accR += (stream[i].ochannels[k][j] * stream[i].chanvol[k]) >> 8;
						break;

					case MIXER_PAN_LEFT:
						accL += (stream[i].ochannels[k][j] * stream[i].chanvol[k]) >> 8;
						break;

					case MIXER_PAN_RIGHT:
						accR += (stream[i].ochannels[k][j] * stream[i].chanvol[k]) >> 8;
						break;
				}
			}
		}

		*out++ = accL;
		*out++ = accR;
	}

	// phase 3: flush the streams back to nothing
	for (i = 0; i < numstreams; i++)
	{
		stream[i].curpos = 0;
	}
}

void mixer_set_volume(int channel,int volume)
{
#if 0
	int i;

	for (i = 0; i < stream[i].numchan; i++)
	{
		stream[channel].chanvol[i] = (MIXER_GET_LEVEL(volume) * 255) / 100;
		printf("ch %d = %d (%d)\n", channel, stream[numstreams].chanvol[i], volume);
	}
#endif
}

char *mixer_get_name(int channel)
{
	return stream[channel].channame[0];
}

void mixer_set_stereo_volume(int ch, int l_vol, int r_vol )
{
	stream[ch].chanvol[0] = l_vol;
	stream[ch].chanvol[1] = r_vol;
}

void mixer_set_stereo_pan(int ch, int panL, int panR)
{
	stream[ch].chanpan[0] = panL;
	stream[ch].chanpan[1] = panR;
}

void mixer_sound_enable_global_w(int enable)
{
	bSoundEnable = enable;
}

void mixer_set_chan_level(int stm, int ch, int vol)
{
	stream[stm].chanvol[ch] = vol;
}

void mixer_set_chan_pan(int stm, int ch, int pan)
{
	stream[stm].chanpan[ch] = pan;
}

int mixer_get_chan_level(int stm, int ch)
{
	return stream[stm].chanvol[ch];
}

int mixer_get_chan_pan(int stm, int ch)
{
	return stream[stm].chanpan[ch];
}

int mixer_get_num_streams(void)
{
	return numstreams;
}

int mixer_get_num_chans(int stm)
{
	return stream[stm].numchan;
}

char *mixer_get_chan_name(int stm, int ch)
{
	return stream[stm].channame[ch];
}

