/* samples handler - very simple playback */

#include "m1stdinc.h"

typedef struct
{
	int sample_active;
	int sample_frac;
	int sample_int;
	int sample_fracptr;
	int sample_intptr;
	int sample_acc;
	int sample_length;
	signed short *sample_base;
} SampleT;

static int num;
static SampleT sample[MAX_SAMPLES];

static void samples_update(int chan, signed short *out, int dspframes);

// global functions

void samples_init(int chans, long sample_rate)
{
	int i;
	char name[40];

	num = chans;

	for (i = 0; i < chans; i++)
	{
		sample[i].sample_active = 0;
		sample[i].sample_frac = 0;
		sample[i].sample_int = 0;
		sample[i].sample_fracptr = 0;
		sample[i].sample_intptr = 0;
		sample[i].sample_acc = 0;
		sample[i].sample_length = 0;
		sample[i].sample_base = (signed short *)NULL;

		sprintf(name, "M1 sample player %d", i);
		stream_init(name, 100, Machine->sample_rate, i, samples_update);
	}
}

void samples_set_info(int chan, int playrate, void *data, int length)
{
	double intr;

	intr = ((double)playrate / (double)Machine->sample_rate) * 65536.0;
	sample[chan].sample_int = ((int) intr)>>16;
	sample[chan].sample_frac = ((int) intr)&0xffff;

	sample[chan].sample_base = data;
	sample[chan].sample_length = length;
}

void samples_play_chan(int chan)
{
	sample[chan].sample_active = 1;

	sample[chan].sample_acc = 0;
	sample[chan].sample_intptr = 0;
	sample[chan].sample_fracptr = 0;
}

void samples_stop_chan(int chan)
{
	sample[chan].sample_active = 0;
}

static void samples_update(int chan, signed short *out, int dspframes)
{
	int i;

	if (sample[chan].sample_active)
	{
		// for each output sample
		for (i = 0; i < dspframes; i++)
		{
			// get the current input sample via the wave pointer
			out[i] = sample[chan].sample_base[sample[chan].sample_acc];

			// adjust the wave pointer by the playback rate
			sample[chan].sample_fracptr += sample[chan].sample_frac;
			sample[chan].sample_acc += sample[chan].sample_int;

			// see if the fraction went over 1.0
			if (sample[chan].sample_fracptr > 0xffff)
			{
				// step the wave pointer by the integer portion of the step
				sample[chan].sample_acc += sample[chan].sample_fracptr>>16;
				// now clear the integer portion of the step
				sample[chan].sample_fracptr &= 0xffff;
			}

			// are we done?
			if (sample[chan].sample_acc >= sample[chan].sample_length)
			{
				sample[chan].sample_active = 0;
				i = dspframes;
			}
		}
	}
	else
	{
		memset(out, 0, dspframes*2);
	}
}
