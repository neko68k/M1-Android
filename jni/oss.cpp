/***************************************************************************
   oss.c  -  M1 Linux audio output driver - supports SDL, OSS, ALSA, and PulseAudio
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "m1snd.h"
#include "oss.h"
#include "wavelog.h"

#define USE_SDL

#ifdef USE_SDL
extern "C" 
{
#include <SDL.h>
}
#endif

// ALSA
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

// PulseAudio
extern "C" 
{
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
}

#define PULSE_USE_SIMPLE	(0)

#define VALGRIND 	(0)	// disables sound output for easier Valgrind usage

#define NUM_FRAGS_BROKEN    (8)
#define NUM_FRAGS_NORMAL    (4)
static INT32 num_frags;
#define OSS_FRAGMENT (0x000D | (num_frags<<16));  // 16k fragments (2 * 2^14).

static audio_buf_info info;

// local variables
void  (*m1sdr_Callback)(unsigned long dwNumSamples, signed short *data);
unsigned long cbUserData;

static int hw_present, oss_pause;

static pa_context *my_pa_context = NULL;
static pa_stream *my_pa_stream = NULL;
static pa_mainloop_api *my_pa_mainloop_api = NULL;
static pa_mainloop* my_pa_mainloop = NULL;

static pa_sample_spec sample_spec;

#if PULSE_USE_SIMPLE
static pa_simple *my_simple;
#endif

static INT32 is_broken_driver;
int nDSoundSegLen = 0;
static int oss_nw = 0, oss_playing = 0;

int audiofd;
static snd_pcm_t *pHandle = NULL;
int lnxdrv_apimode = 3;		// 0 = SDL, 1 = ALSA, 2 = OSS, 3 = PulseAudio

static int playtime = 0;

static INT16 samples[(48000*2)];

#define kMaxBuffers (4)		// this adjusts the latency for SDL audio

static volatile INT16 *buffer[kMaxBuffers];
static volatile int bufstat[kMaxBuffers];
static int playbuf, writebuf;
static UINT8 *curpos;
static int bytes_left;

static void fill_buffer(int bufnum)
{
	int bytes_to_fill, bufpos;
	UINT8 *bufptr;

	//	printf("FB%d\n", bufnum);

	// figure out how much we need out of this buffer
	// vs how much we can get out of it
	if (bytes_left >= bufstat[bufnum])
	{
		bytes_to_fill = bufstat[bufnum];
	}
	else
	{
		bytes_to_fill = bytes_left;
	}

	// copy from the buffer's current position
	bufptr = (UINT8 *)buffer[bufnum];
	bufpos = (nDSoundSegLen*2*sizeof(INT16)) - bufstat[bufnum];
	bufptr += bufpos;
	memcpy(curpos, bufptr, bytes_to_fill);

	// reduce the counters
	curpos += bytes_to_fill;
	bufstat[bufnum] -= bytes_to_fill;
	bytes_left -= bytes_to_fill;
}

static void sdl_callback(void *userdata, UINT8 *stream, int len)
{
	int temp;

	curpos = stream;
	bytes_left = len;

	// need more data?
	while (bytes_left > 0)
	{
		// does our current buffer have any samples?
		if (bufstat[playbuf] > 0)
		{
			fill_buffer(playbuf);
		}
		else
		{
			// check if the next buffer would collide
			temp = playbuf + 1;
			if (temp >= kMaxBuffers)
			{
				temp = 0;
			}

			// no collision, set it and continue looping
			if (temp != writebuf)
			{
				playbuf = temp;
			}
			else
			{
//			  printf("UF\n");
				// underflow!
				memset(curpos, 0, bytes_left);
				bytes_left = 0;
			}
		}
	}
}

// set # of samples per update
void m1sdr_SetSamplesPerTick(UINT32 spf)
{
	int i;

	nDSoundSegLen = spf;

	if (lnxdrv_apimode == 0) 
	{
		for (i = 0; i < kMaxBuffers; i++)
		{
			if (buffer[i])
			{
				free((void *)buffer[i]);
				buffer[i] = (volatile INT16 *)NULL;
			}

			buffer[i] = (volatile INT16 *)malloc(nDSoundSegLen * 2 * sizeof(UINT16));
			if (!buffer[i])
			{
				printf("Couldn't alloc buffer for SDL audio!\n");
				exit(-1);
			}

			memset((void *)buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));

			bufstat[i] = 0;
		}

		playbuf = 0;
		writebuf = 1;
	}
}

// m1sdr_Update - timer callback routine: runs sequencer and mixes sound
void m1sdr_Update(void)
{	
	if ((m1sdr_Callback) && (!oss_pause))
	{
		if (lnxdrv_apimode == 0) 
		{
	        	m1sdr_Callback(nDSoundSegLen, (INT16 *)buffer[writebuf]);
			memcpy(samples, (const void *)buffer[writebuf], nDSoundSegLen*4);
	
			bufstat[writebuf] = nDSoundSegLen * 2 * sizeof(UINT16);

			if (++writebuf >= kMaxBuffers)
			{
				writebuf = 0;
			}
		}
		else
		{
			m1sdr_Callback(nDSoundSegLen, (INT16 *)samples);
		}
	}

	if (oss_pause)
	{
		memset(samples, 0, nDSoundSegLen*4);
	}
}

// checks the play position to see if we should trigger another update
void m1sdr_TimeCheck(void)
{
//	int timeout;
	snd_pcm_sframes_t avail = 0;

#if VALGRIND
	m1sdr_Update();
#else
	switch (lnxdrv_apimode)
	{

	case 0:	// SDL
		#ifdef USE_SDL
		SDL_LockAudio();

		while ((bufstat[writebuf] == 0) && (writebuf != playbuf))
		{
			m1sdr_Update();
			playtime++;
		}

		SDL_UnlockAudio();
		#endif
		break;  

	case 1:	// ALSA
		if ((!pHandle) || (!oss_playing))
		{
			m1sdr_Update();
			playtime++;
			return;
		}
		avail = snd_pcm_avail_update(pHandle);

//		printf("avail %d\n", avail);
#if 0
		if (avail < 0)
		{
			int err;

			if ((err = snd_pcm_prepare(pHandle)) < 0) {
				fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
				return 0;
			}
		}
#endif
		while (avail >= nDSoundSegLen)
		{
			int status;

			m1sdr_Update();
			waveLogFrame((unsigned char *)samples, nDSoundSegLen * 4);
			playtime++;

			status = snd_pcm_writei(pHandle, samples, nDSoundSegLen);
			if (status < 0)
			{
				if (status == -EAGAIN) 
				{ 
//					printf("EAGAIN\n");
					do
					{
						status = snd_pcm_resume(pHandle);
					} while (status == -EAGAIN);
				}

				if (status == -ESTRPIPE) 
				{ 
//					printf("ESTRPIPE\n");
					do
					{
						status = snd_pcm_resume(pHandle);
					} while (status == -EAGAIN);
				}
				if (status < 0)
				{
					status = snd_pcm_prepare(pHandle);
				}
			}


			avail = snd_pcm_avail_update(pHandle);
		}
		break;	

	case 2:	// OSS
		if ((audiofd == -1) || (!oss_playing))
		{
			m1sdr_Update();
			playtime++;
			return;
		}

	    	ioctl(audiofd, SNDCTL_DSP_GETOSPACE, &info);

		if (oss_nw)
		{
			int err;

			m1sdr_Update();

			// output the generated samples
			err = write(audiofd, samples, nDSoundSegLen * 4);
			if (err == -1)
			{
				perror("write\n");
			}

			waveLogFrame((unsigned char *)samples, nDSoundSegLen * 4);
		}
		else
		{
		    	while (info.bytes >= (nDSoundSegLen * 4))
			{
				m1sdr_Update();
				playtime++;

				// output the generated samples
				write(audiofd, samples, nDSoundSegLen * 4);

				waveLogFrame((unsigned char *)samples, nDSoundSegLen * 4);

			    	ioctl(audiofd, SNDCTL_DSP_GETOSPACE, &info);
			}
		}
		break;

	case 3:	// PulseAudio
		{
			#if !PULSE_USE_SIMPLE
			int size;

			if ((!my_pa_context) || (!my_pa_stream) || (!my_pa_mainloop))
			{
				return;
			}

			while (1)
			{
				if (pa_context_get_state(my_pa_context) != PA_CONTEXT_READY ||
				    pa_stream_get_state(my_pa_stream) != PA_STREAM_READY ||
				    pa_mainloop_iterate(my_pa_mainloop, 0, NULL) < 0)
				{
					usleep(50);
			    		return;
				}

				size = pa_stream_writable_size(my_pa_stream);
//				printf("stream has %d bytes avail, want %d\n", size, nDSoundSegLen*4);

				if (size >= (nDSoundSegLen*4))
				{
					m1sdr_Update();
					playtime++;
					waveLogFrame((unsigned char *)samples, nDSoundSegLen * 4);

					if (pa_stream_write(my_pa_stream, (uint8_t*)samples, nDSoundSegLen*4, NULL, 0, PA_SEEK_RELATIVE) < 0)
					{
//						printf("pa_stream_write() failed: %s\n", pa_strerror(pa_context_errno(my_pa_context)));
					}
				}
				else
				{
					usleep(50);
					return;
				}
			}
			#else
			m1sdr_Update();
			playtime++;
			waveLogFrame((unsigned char *)samples, nDSoundSegLen * 4);

			pa_simple_write(my_simple, samples, nDSoundSegLen*4, NULL);
			#endif
		}
		break;
	}

	usleep(50);
#endif
}

// m1sdr_Init - inits the output device and our global state

INT16 m1sdr_Init(int sample_rate)
{	
	int format, stereo, rate, fsize, err, state;
	unsigned int nfreq, periodtime;
	snd_pcm_hw_params_t *hwparams;
	#ifdef USE_SDL
	SDL_AudioSpec aspec;
	#endif
	pa_channel_map chanmap;
	pa_buffer_attr my_pa_attr;

	hw_present = 0;

	m1sdr_Callback = NULL;

	nDSoundSegLen = sample_rate / 60;

	switch (lnxdrv_apimode)
	{
	case 0: // SDL
		#ifdef USE_SDL
		SDL_InitSubSystem(SDL_INIT_AUDIO);

	 	m1sdr_SetSamplesPerTick(sample_rate/60);

		playbuf = 0;
		writebuf = 1;

		aspec.freq = sample_rate;
		aspec.format = AUDIO_S16SYS;	// keep endian independant 
		aspec.channels = 2;
		aspec.samples = 512;		// has to be a power of 2, and we want it smaller than our buffer size
		aspec.callback = sdl_callback;
		aspec.userdata = 0;

		if (SDL_OpenAudio(&aspec, NULL) < 0)
		{
			printf("ERROR: can't open SDL audio\n");
			return 0;
		}

		// make sure we don't start yet
		SDL_PauseAudio(1);
		#endif
		break;
				
	case 1:	// ALSA
		// Try to open audio device
		if ((err = snd_pcm_open(&pHandle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			fprintf(stderr, "ALSA: Could not open soundcard (%s)\n", snd_strerror(err));
			hw_present = 0;
			return 0;
		}

		if ((err = snd_pcm_hw_params_malloc(&hwparams)) < 0) {
			fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
				 snd_strerror(err));
			return 0;
		}

		// Init hwparams with full configuration space
		if ((err = snd_pcm_hw_params_any(pHandle, hwparams)) < 0) {
			fprintf(stderr, "ALSA: couldn't set hw params (%s)\n", snd_strerror(err));
			hw_present = 0;
			return 0;
		}

		// Set access type
		if ((err = snd_pcm_hw_params_set_access(pHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			fprintf(stderr, "ALSA: can't set access (%s)\n", snd_strerror(err));
			return 0;
		}

		// Set sample format
		if ((err = snd_pcm_hw_params_set_format(pHandle, hwparams, SND_PCM_FORMAT_S16)) < 0) {
			fprintf(stderr, "ALSA: can't set format (%s)\n", snd_strerror(err));
			return 0;
		}

		// Set sample rate (nearest possible)
		nfreq = sample_rate;
		if ((err = snd_pcm_hw_params_set_rate_near(pHandle, hwparams, &nfreq, 0)) < 0) {
			fprintf(stderr, "ALSA: can't set sample rate (%s)\n", snd_strerror(err));
			return 0;
		}

		// Set number of channels
		if ((err = snd_pcm_hw_params_set_channels(pHandle, hwparams, 2)) < 0) {
			fprintf(stderr, "ALSA: can't set stereo (%s)\n", snd_strerror(err));
			return 0;
		}

		// Set period time (nearest possible)
		periodtime = 16;
		if ((err = snd_pcm_hw_params_set_period_time_near(pHandle, hwparams, &periodtime, 0)) < 0) {
			fprintf(stderr, "ALSA: can't set period time (%s)\n", snd_strerror(err));
			return 0;
		}

		// Apply HW parameter settings to PCM device and prepare device
		if ((err = snd_pcm_hw_params(pHandle, hwparams)) < 0) {
			fprintf(stderr, "ALSA: unable to install hw_params (%s)\n", snd_strerror(err));
			snd_pcm_hw_params_free(hwparams);
			return 0;
		}

		snd_pcm_hw_params_free(hwparams);

		if ((err = snd_pcm_prepare(pHandle)) < 0) {
			fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
			return 0;
		}
		break;	

	case 2:	// OSS
		audiofd = open("/dev/dsp", O_WRONLY, 0);
		if (audiofd == -1)
		{
			audiofd = open("/dev/dsp1", O_WRONLY, 0);

			if (audiofd == -1)
			{
				perror("/dev/dsp1");
				return(0);
			}
		}

		// reset things
		ioctl(audiofd, SNDCTL_DSP_RESET, 0);

		is_broken_driver = 0;
		num_frags = NUM_FRAGS_NORMAL;

		// set the buffer size we want
		fsize = OSS_FRAGMENT;
		if (ioctl(audiofd, SNDCTL_DSP_SETFRAGMENT, &fsize) == - 1)
		{
			perror("SNDCTL_DSP_SETFRAGMENT");
			return(0);
		}

		// set 16-bit output
		format = AFMT_S16_NE;	// 16 bit signed "native"-endian
		if (ioctl(audiofd, SNDCTL_DSP_SETFMT, &format) == - 1)
		{
			perror("SNDCTL_DSP_SETFMT");
			return(0);
		}

		// now set stereo
		stereo = 1;
		if (ioctl(audiofd, SNDCTL_DSP_STEREO, &stereo) == - 1)
		{
			perror("SNDCTL_DSP_STEREO");
			return(0);
		}

		// and the sample rate
		rate = sample_rate;
		if (ioctl(audiofd, SNDCTL_DSP_SPEED, &rate) == - 1)
		{
			perror("SNDCTL_DSP_SPEED");
			return(0);
		}

		// and make sure that did what we wanted
		ioctl(audiofd, SNDCTL_DSP_GETBLKSIZE, &fsize);
		break;

	case 3: // PulseAudio
		sample_spec.format = PA_SAMPLE_S16NE;
		sample_spec.rate = sample_rate;
		sample_spec.channels = 2;

		my_pa_context = NULL;
		my_pa_stream = NULL;
		my_pa_mainloop = NULL;
		my_pa_mainloop_api = NULL;

		#if !PULSE_USE_SIMPLE
		// get default channel mapping
		pa_channel_map_init_auto(&chanmap, sample_spec.channels, PA_CHANNEL_MAP_WAVEEX);


		if (!(my_pa_mainloop = pa_mainloop_new()))
		{
			fprintf(stderr, "pa_mainloop_new() failed\n");
			return 0;
		}

		my_pa_mainloop_api = pa_mainloop_get_api(my_pa_mainloop);

/*		if (pa_signal_init(my_pa_mainloop_api) != 0)
		{
			fprintf(stderr, "pa_signal_init() failed\n");
			return 0;
		}*/

		/* Create a new connection context */
		if (!(my_pa_context = pa_context_new(my_pa_mainloop_api, "Audio Overload"))) 
		{
			fprintf(stderr, "pa_context_new() failed\n");
			return 0;
		}

		/* set the context state CB */
//		pa_context_set_state_callback(my_pa_context, context_state_callback, NULL);

		/* Connect the context */
		if (pa_context_connect(my_pa_context, NULL, (pa_context_flags_t)0, NULL) < 0)
		{
			fprintf(stderr, "pa_context_connect() failed: %s", pa_strerror(pa_context_errno(my_pa_context)));
			return 0;
		}
		
		do
		{
			pa_mainloop_iterate(my_pa_mainloop, 1, NULL);
			state = pa_context_get_state(my_pa_context);
			if (!PA_CONTEXT_IS_GOOD((pa_context_state_t)state))
			{
				printf("PA CONTEXT NOT GOOD\n");
				hw_present = 0;
				return 0;
			}
		} while (state != PA_CONTEXT_READY);

		if (!(my_pa_stream = pa_stream_new(my_pa_context, "Audio Overload", &sample_spec, &chanmap))) 
		{
			fprintf(stderr, "pa_stream_new() failed: %s\n", pa_strerror(pa_context_errno(my_pa_context)));
			return 0;
		}

		memset(&my_pa_attr, 0, sizeof(my_pa_attr));
		my_pa_attr.tlength = nDSoundSegLen * 4 * 4;
		my_pa_attr.prebuf = -1;
		my_pa_attr.maxlength = -1;
		my_pa_attr.minreq = nDSoundSegLen * 4 * 2;

		if ((err = pa_stream_connect_playback(my_pa_stream, NULL, &my_pa_attr, PA_STREAM_ADJUST_LATENCY, NULL, NULL)) < 0)
		{
			fprintf(stderr, "pa_stream_connect_playback() failed: %s\n", pa_strerror(pa_context_errno(my_pa_context)));
			return 0;
		}

		do
		{
			pa_mainloop_iterate(my_pa_mainloop, 1, NULL);
			state = pa_stream_get_state(my_pa_stream);
			if (!PA_STREAM_IS_GOOD((pa_stream_state_t)state))
			{
				printf("PA STREAM NOT GOOD\n");
				hw_present = 0;
				return 0;
			}
		} while (state != PA_STREAM_READY);

//		printf("PulseAudio setup OK so far, len %d\n", nDSoundSegLen*4);
		#else
		my_simple = NULL;
		#endif
		break;
	}

	hw_present = 1;

	return (1);
}

void m1sdr_Exit(void)
{	
	int i;

	if (!hw_present) return;

	switch (lnxdrv_apimode)
	{
	case 0:	// SDL
		#ifdef USE_SDL
		SDL_QuitSubSystem(SDL_INIT_AUDIO);

		for (i = 0; i < kMaxBuffers; i++)
		{
			if (buffer[i])
			{
				free((void *)buffer[i]);
				buffer[i] = (volatile INT16 *)NULL;
			}
		}
		#endif
		break;	

	case 1:	// ALSA
//		printf("ALSA kill handle %x\n", pHandle);
		if (pHandle > 0)
		{
			snd_pcm_close(pHandle);
		}
		break;	

	case 2:	// OSS
		close(audiofd);
		break;	

	case 3:	// PulseAudio
		if (my_pa_stream)
		{
			pa_stream_disconnect(my_pa_stream);
			pa_stream_unref(my_pa_stream);
		}

		if (my_pa_context)
		{
			pa_context_disconnect(my_pa_context);
			pa_context_unref(my_pa_context);
		}

//		pa_signal_done();

		if (my_pa_mainloop)
		{
			pa_mainloop_free(my_pa_mainloop);
		}

		#if PULSE_USE_SIMPLE
		if (my_simple)
		{
			pa_simple_flush(my_simple, NULL);
			pa_simple_free(my_simple);
		}
	
		my_simple = NULL;
		#endif

		my_pa_context = NULL;
		my_pa_stream = NULL;
		my_pa_mainloop = NULL;
		my_pa_mainloop_api = NULL;
		break;
	}
}

void m1sdr_SetCallback(void *fn)
{
	if (fn == (void *)NULL)
	{
		printf("ERROR: NULL CALLBACK!\n");
	}

	m1sdr_Callback = (void (*)(unsigned long, signed short *))fn;
}

INT16 m1sdr_IsThere(void)
{
	int err;

	if (lnxdrv_apimode == 1)
	{
		if ((err = snd_pcm_open(&pHandle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0)) != 0)
		{
			printf("Error accessing soundcard, sound will be disabled\n");
			hw_present = 0;
			return(0);
		}

		snd_pcm_close(pHandle);
		hw_present = 1;
	}
	else if (lnxdrv_apimode == 2)
	{
		audiofd = open("/dev/dsp", O_WRONLY, 0);

		if (audiofd == -1)
		{
			printf("Error accessing soundcard, sound will be disabled\n");
			hw_present = 0;
			return(0);
		}

		close(audiofd);
		hw_present = 1;
	}
	else if (lnxdrv_apimode == 0)
	{
		hw_present = 1;	// always say it's present for SDL
	}

	return (1);
}

INT32 m1sdr_HwPresent(void)
{
	return hw_present;
}

void m1sdr_PlayStart(void)
{
	#ifdef USE_SDL
	if (lnxdrv_apimode == 0) 
	{
		int i;

		for (i = 0; i < kMaxBuffers; i++)
		{
			if (buffer[i])
			{
				memset((void *)buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
			}
		}


		SDL_PauseAudio(0);
	}
	#endif

	if (lnxdrv_apimode == 1)
	{
		snd_pcm_start(pHandle);
		snd_pcm_prepare(pHandle);
	}

	if (lnxdrv_apimode == 3)
	{
		#if PULSE_USE_SIMPLE
		my_simple = pa_simple_new(
			NULL, 
			"Audio Overload",
			PA_STREAM_PLAYBACK, 
			0,               
                	"audio",
			&sample_spec,
			NULL, 
			NULL,
			NULL);
		#endif
	}

	waveLogStart();
	oss_playing = 1;
}

//static void pa_stream_drain_complete(pa_stream *s, int success, void *userdata)
//{
//}

void m1sdr_PlayStop(void)
{
	#if 0
	pa_operation *op;
	#endif

	#ifdef USE_SDL
	if (lnxdrv_apimode == 0) 
	{
		SDL_PauseAudio(1);
	}
	#endif

	if (lnxdrv_apimode == 1)
	{
//		snd_pcm_pause(pHandle, 1);
		snd_pcm_drop(pHandle);
	}

	#if PULSE_USE_SIMPLE
	if ((lnxdrv_apimode == 3) && (my_simple))
	{
		pa_simple_flush(my_simple, NULL);
		pa_simple_free(my_simple);
		my_simple = NULL;
	}
	#else
#if 0
	if (lnxdrv_apimode == 3)
	{
		op = pa_stream_drain(my_pa_stream, &pa_stream_drain_complete, NULL);
		if (op)
		{
			while (pa_operation_get_state(op) != PA_OPERATION_DONE)
			{
				if (pa_context_get_state(my_pa_context) != PA_CONTEXT_READY ||
				    pa_stream_get_state(my_pa_stream) != PA_STREAM_READY ||
				    pa_mainloop_iterate(my_pa_mainloop, 0, NULL) < 0)
				    {
				    	pa_operation_cancel(op);
					break;
				    }
			}
		}
	}
#endif
	#endif

	waveLogStop();
	oss_playing = 0;
}

void m1sdr_FlushAudio(void)
{
	memset(samples, 0, nDSoundSegLen * 4);
	if (lnxdrv_apimode == 2) 
	{
		write(audiofd, samples, nDSoundSegLen * 4);
		write(audiofd, samples, nDSoundSegLen * 4);
	}
}

void m1sdr_Pause(int set)
{
	oss_pause = set;
}

void m1sdr_SetNoWait(int nw)
{
	oss_nw = nw;
}

short *m1sdr_GetSamples(void)
{
	return samples;
}

int m1sdr_GetPlayTime(void)
{
	return playtime;
}

