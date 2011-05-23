/***************************************************************************

	DMA-driven DAC driver
	by Aaron Giles

***************************************************************************/

#include "driver.h"
#include "dmadac.h"



/*************************************
 *
 *	Debugging
 *
 *************************************/

#define VERBOSE		0

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif



/*************************************
 *
 *	Constants
 *
 *************************************/

#define FRAC_BITS					24
#define FRAC_ONE					(1 << FRAC_BITS)
#define FRAC_INT(a)					((a) >> FRAC_BITS)
#define FRAC_FRAC(a)				((a) & (FRAC_ONE - 1))

#define BUFFER_SIZE					32768
#define SECONDS_BETWEEN_ADJUSTS		1
#define ADJUSTS_TO_QUIESCE			1



/*************************************
 *
 *	Types
 *
 *************************************/

struct dmadac_channel_data
{
	/* sound stream and buffers */
	int		channel;
	INT16 *	buffer;
	INT16	last;
	
	/* per-channel parameters */
	INT16	volume;
	UINT8	enabled;
	double	frequency;
	
	/* info for sample rate conversion */
	UINT32	step;
	UINT32	curoutfrac;
	UINT32	curoutpos;
	UINT32	curinpos;
	
	/* info for tracking against the system sample rate */
	UINT32	outsamples;
	UINT32	shortages;
	UINT32	overruns;
};



/*************************************
 *
 *	Internal globals
 *
 *************************************/

static double freqmult;
static UINT32 freqmult_quiece_time;
static UINT32 consecutive_shortages;
static UINT32 consecutive_overruns;

static struct dmadac_channel_data dmadac[MAX_DMADAC_CHANNELS];



/*************************************
 *
 *	Step value computation
 *
 *************************************/

INLINE void compute_step(struct dmadac_channel_data *ch)
{
	ch->step = (UINT32)(((ch->frequency * freqmult) / (double)Machine->sample_rate) * (double)FRAC_ONE);
}



/*************************************
 *
 *	Periodically adjust the effective
 *	frequency
 *
 *************************************/

static void adjust_freqmult(void)
{
	int shortages = 0;
	int overruns = 0;
	int i;
	
	/* first, sum up the data for all channels */
	for (i = 0; i < MAX_DMADAC_CHANNELS; i++)
		if (dmadac[i].outsamples)
		{
			shortages += dmadac[i].shortages;
			overruns += dmadac[i].overruns;
			dmadac[i].shortages = 0;
			dmadac[i].overruns = 0;
			dmadac[i].outsamples -= Machine->sample_rate * SECONDS_BETWEEN_ADJUSTS;
		}
	
	/* don't do anything if we're quiescing */
	if (freqmult_quiece_time)
	{
		freqmult_quiece_time--;
		return;
	}

	/* if we've been short, we need to reduce the effective sample rate so that we stop running out */
	if (shortages)
	{
		consecutive_shortages++;
		freqmult -= 0.00001 * consecutive_shortages;
		LOG(("adjust_freqmult: %d shortages, %d consecutive, decrementing freqmult to %f\n", shortages, consecutive_shortages, freqmult));
		freqmult_quiece_time = ADJUSTS_TO_QUIESCE;
	}
	else
		consecutive_shortages = 0;
	
	/* if we've been over, we need to increase the effective sample rate so that we consume the data faster */
	if (overruns)
	{
		consecutive_overruns++;
		freqmult += 0.00001 * consecutive_overruns;
		LOG(("adjust_freqmult: %d overruns, %d consecutive, incrementing freqmult to %f\n", overruns, consecutive_overruns, freqmult));
		freqmult_quiece_time = ADJUSTS_TO_QUIESCE;
	}
	else
		consecutive_overruns = 0;
	
	/* now recompute the step value for each channel */
	for (i = 0; i < MAX_DMADAC_CHANNELS; i++)
		compute_step(&dmadac[i]);
}



/*************************************
 *
 *	Stream callback
 *
 *************************************/

static void dmadac_update(int num, INT16 *buffer, int length)
{
	struct dmadac_channel_data *ch = &dmadac[num];
	UINT32 frac = ch->curoutfrac;
	UINT32 out = ch->curoutpos;
	UINT32 step = ch->step;
	INT16 last = ch->last;
	
	/* track how many samples we've been asked to output; every second, consider adjusting */
	ch->outsamples += length;
	if (ch->outsamples >= Machine->sample_rate * SECONDS_BETWEEN_ADJUSTS)
		adjust_freqmult();
	
	/* if we're not enabled, just fill with silence */
	if (!ch->enabled)
	{
		while (length-- > 0)
			*buffer++ = 0;
		return;
	}

	LOG(("dmadac_update(%d) - %d to consume, %d effective, %d in buffer, ", num, length, (int)(length * ch->frequency / (double)Machine->sample_rate), ch->curinpos - ch->curoutpos));

	/* fill with data while we can */	
	while (length > 0 && out < ch->curinpos)
	{
		INT32 tmult = frac >> (FRAC_BITS - 8);
		last = ((ch->buffer[(out - 1) % BUFFER_SIZE] * (0x100 - tmult)) + (ch->buffer[out % BUFFER_SIZE] * tmult)) >> 8;
		*buffer++ = (last * ch->volume) >> 8;
		frac += step;
		out += FRAC_INT(frac);
		frac = FRAC_FRAC(frac);
		length--;
	}
	
	/* trail the last byte if we run low */
	if (length > 0)
	{
		LOG(("dmadac_update: short by %d samples\n", length));
		ch->shortages++;
	}
	while (length-- > 0)
		*buffer++ = (last * ch->volume) >> 8;
	
	/* update the values */
	ch->curoutfrac = frac;
	ch->curoutpos = out;
	ch->last = last;
	
	/* adjust for wrapping */
	while (ch->curoutpos > BUFFER_SIZE && ch->curinpos > BUFFER_SIZE)
	{
		ch->curoutpos -= BUFFER_SIZE;
		ch->curinpos -= BUFFER_SIZE;
	}
	LOG(("%d left\n", ch->curinpos - ch->curoutpos));
}



/*************************************
 *
 *	Sound hardware init
 *
 *************************************/

int dmadac_sh_start(const struct MachineSound *msound)
{
	const struct dmadac_interface *intf = msound->sound_interface;
	int i;

	if (Machine->sample_rate == 0)
		return 0;

	/* init globals */
	freqmult = 1.0;
	freqmult_quiece_time = 0;
	consecutive_shortages = 0;
	consecutive_overruns = 0;
	
	/* init each channel */
	for (i = 0; i < intf->num; i++)
	{
		char name[40];
		
		/* allocate a clear a buffer */
		dmadac[i].buffer = auto_malloc(sizeof(dmadac[i].buffer[0]) * BUFFER_SIZE);
		if (!dmadac[i].buffer)
			return 1;
		memset(dmadac[i].buffer, 0, sizeof(dmadac[i].buffer[0]) * BUFFER_SIZE);
		
		/* reset the state */
		dmadac[i].last = 0;
		dmadac[i].volume = 0x100;
		dmadac[i].enabled = 0;
		
		/* reset the framing */
		dmadac[i].step = 0;
		dmadac[i].curoutfrac = 0;
		dmadac[i].curoutpos = 0;
		dmadac[i].curinpos = 0;

		/* allocate a stream channel */
		sprintf(name, "DMA DAC #%d", i);
		dmadac[i].channel = stream_init(name, intf->mixing_level[i], Machine->sample_rate, i, dmadac_update);
		if (dmadac[i].channel == -1)
			return 1;
	}

	return 0;
}



/*************************************
 *
 *	Primary transfer routine
 *
 *************************************/

void dmadac_transfer(UINT8 first_channel, UINT8 num_channels, offs_t channel_spacing, offs_t frame_spacing, offs_t total_frames, INT16 *data)
{
	int i, j;
	
	if (Machine->sample_rate == 0)
		return;
	
	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
		stream_update(dmadac[first_channel + i].channel, 0);
	
	/* loop over all channels and accumulate the data */
	for (i = 0; i < num_channels; i++)
	{
		struct dmadac_channel_data *ch = &dmadac[first_channel + i];
		if (ch->enabled)
		{
			INT16 *src = data + i * channel_spacing;
			offs_t frames_to_copy = total_frames;
			offs_t in = ch->curinpos;
			offs_t space_in_buf = (BUFFER_SIZE - 1) - (in - ch->curoutpos);
			
			/* are we overrunning? */
			if (in - ch->curoutpos > total_frames)
				ch->overruns++;
			
			/* if we want to copy in too much data, clamp to the maximum that will fit */
			if (space_in_buf < frames_to_copy)
			{
				LOG(("dmadac_transfer: attempted %d frames to channel %d, but only %d fit\n", frames_to_copy, first_channel + i, space_in_buf));
				frames_to_copy = space_in_buf;
			}

			/* copy the data */
			for (j = 0; j < frames_to_copy; j++)
			{
				ch->buffer[in++ % BUFFER_SIZE] = *src;
				src += frame_spacing;
			}
			ch->curinpos = in;
		}
	}

	LOG(("dmadac_transfer - %d samples, %d effective, %d in buffer\n", total_frames, (int)(total_frames * (double)Machine->sample_rate / dmadac[first_channel].frequency), dmadac[first_channel].curinpos - dmadac[first_channel].curoutpos));
}



/*************************************
 *
 *	Enable/disable DMA channel(s)
 *
 *************************************/

void dmadac_enable(UINT8 first_channel, UINT8 num_channels, UINT8 enable)
{
	int i;
	
	if (Machine->sample_rate == 0)
		return;
	
	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		stream_update(dmadac[first_channel + i].channel, 0);
		dmadac[first_channel + i].enabled = enable;
		if (!enable)
			dmadac[first_channel + i].curinpos = dmadac[first_channel + i].curoutpos = dmadac[first_channel + i].curoutfrac = 0;
	}
}



/*************************************
 *
 *	Set the frequency on DMA channel(s)
 *
 *************************************/

void dmadac_set_frequency(UINT8 first_channel, UINT8 num_channels, double frequency)
{
	int i;
	
	if (Machine->sample_rate == 0)
		return;
	
	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		stream_update(dmadac[first_channel + i].channel, 0);
		dmadac[first_channel + i].frequency = frequency;
		compute_step(&dmadac[first_channel + i]);
	}
}



/*************************************
 *
 *	Set the volume on DMA channel(s)
 *
 *************************************/

void dmadac_set_volume(UINT8 first_channel, UINT8 num_channels, UINT16 volume)
{
	int i;
	
	if (Machine->sample_rate == 0)
		return;
	
	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		stream_update(dmadac[first_channel + i].channel, 0);
		dmadac[first_channel + i].volume = volume;
	}
}



