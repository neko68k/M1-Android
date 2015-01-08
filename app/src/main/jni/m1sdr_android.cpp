#include <assert.h>
#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include "m1ui.h"
#include "m1snd.h"
#include "wavelog.h"
#include "m1sdr_android.h"
#include "thread_timer.h"


// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
// #include <android/log.h>



// for native asset manager
#include <sys/types.h>
//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>

#define kMaxBuffers			4

static INT32 is_broken_driver;
int nDSoundSegLen = 0;
static int oss_nw = 0, oss_playing = 0;

static INT16 samples[(44100*2)];

/*static volatile INT16 *buffer[kMaxBuffers];
static volatile int bufstat[kMaxBuffers];
static int playbuf, writebuf;
static UINT8 *curpos;
static int bytes_left;*/


int hw_present = 0;


static void	*buffer, *playBuffer;
static int	buffers_used = 0, buffers_read = 0, buffers_write = 0;
static int	sdr_pause = 0;
static bool	playing = false;
static int	playtime = 0;

//void m1sdr_Exit(void);
//void m1sdr_StopTimer(void);
//void m1sdr_GenerationCallback();

void  (*m1sdr_Callback)(unsigned long dwNumSamples, signed short *data);

// m1sdr_Init - inits the output device and our global state

INT16 m1sdr_Init(int sample_rate)
{
	hw_present = 1;
	return (1);
}


void m1sdr_Exit(void)
{
	/*if (channel != nil)
	{
		SndDisposeChannel(channel, true);
		channel = nil;
	}

	if (callback != nil)
	{
		DisposeSndCallBackUPP(callback);
		callback = nil;
	}

	if (header != nil)
	{
		DisposePtr((Ptr)header);
		header = nil;
	}*/	
}


//
// m1sdr_PlayCallback
// Copies pregenerated sound buffers to output device
//
// call from UpdateThread
void Java_com_neko68k_M1_NDKBridge_m1sdrPlayCallback()
{
	// i think I can eliminate this call on the native side
	// by passing the new buffer to the java side in the
	// generation callback below
	// then I can keep the buffer count on the java side
	// instead of the native side, this is probably
	// best for keeping the threads synchronized
	/*if (buffers_used > 0)
	{
		memcpy(playBuffer, buffer[buffers_read], nDSoundSegLen * 2 * sizeof(UINT16));
		if (++buffers_read >= kMaxBuffers) buffers_read = 0;
		//buffers_used--;
		playtime++;
	}
	else
	{
		memset(playBuffer, 0, header->numFrames * 2 * sizeof(UINT16));
	}
	return(playBuffer);*/
}
void waitForBoot(){
	m1sdr_Callback(0, (short *)buffer);
}

// call from update thread
// this needs to return the buffer to Java
// Java will keep it in the Semaphore and
// track all the counts on that end instead of
// this one
jbyteArray m1sdrGenerationCallback(JNIEnv *env)
{

	jbyteArray jb;

	//if (sdr_pause)
	//	return;
	//__android_log_print(ANDROID_LOG_INFO, "M1Android", "Buffering...");
	// Render the oscilloscope approximately 30fps
	/*if ((TickCount() - last_draw) >= 2)
	{
		last_draw = TickCount();
		window_draw_scope();
	}*/

	// Buffer some more audio
	//while (playing && (buffers_used < (kMaxBuffers - 1)))
	//{



	m1sdr_Callback(nDSoundSegLen, (short *)buffer);
	jb=env->NewByteArray(nDSoundSegLen * 2 * 2);
	env->SetByteArrayRegion(jb, 0,
			nDSoundSegLen * 2 * 2, (jbyte *)buffer);
	return(jb);
		//waveLogFrame((unsigned char *)buffer[buffers_write], nDSoundSegLen << 2);

		//if (++buffers_write >= kMaxBuffers) buffers_write = 0;
		//buffers_used++;
		//gettimeofday(&nextTime, NULL);
		/*if ((nextTime.tv_sec - nextTime.tv_sec < 60) && (buffers_used < (kMaxBuffers >> 1)))
		{
			// do a one shot update
			updateTimerWait(0);
			return;
		}*/

}


//
// m1sdr_GetPlayTime
// Returns time of current playback in seconds
//

int m1sdr_GetPlayTime(void)
{
	return (playtime / 60);
}


//
// m1sdr_GetPlayTimeStr
// Returns string of current playback time in minutes and seconds
//

void m1sdr_GetPlayTimeStr(char *buf)
{
	int seconds, minutes;
	
	seconds = m1sdr_GetPlayTime();
	minutes = (seconds / 60);
	seconds -= minutes * 60;
	
	sprintf(buf, "    %i:%.2i", minutes, seconds);
}


//
// m1sdr_Pause
// Temporarily halt playback
//

void m1sdr_Pause(int p)
{
	sdr_pause = p;
}


//
// m1sdr_GetPlayBufferPtr
// Returns a buffer to the current playback buffer. This may change at any time
// and is only recommended for oscilloscope generation or the like. Do not attempt
// to record the output of this function!
//

INT16 *m1sdr_GetPlayBufferPtr(void) 
{
	return (INT16 *)playBuffer;
}

//
// m1sdr_GenerationCallback
// Routine which actually generates audio which our play code will get hold of later
//



//
// m1sdr_CurrentlyPlaying
// Returns a boolean indicating if we're operational or not
//

bool m1sdr_CurrentlyPlaying(void)
{
	return playing;
}


//
// m1sdr_ClearBuffers
// Reset the sound buffers to their empty state
//

static void m1sdr_ClearBuffers(void)
{
	/*int i;
	
	for (i = 0; i < kMaxBuffers; i++)
	{
		memset(buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
	}
	
	memset(playBuffer, 0, nDSoundSegLen * 2 * sizeof(UINT16));
	
	buffers_used = buffers_read = buffers_write = 0;*/
}


//
// m1sdr_PlayStart
// Start the callback
//

void m1sdr_PlayStart(void)
{
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Hit play start...");
	// Stop any existing sound
	m1sdr_PlayStop();
	
	// Clear buffers
	m1sdr_ClearBuffers();
	//header->samplePtr = (char *)playBuffer;
	playtime = 0;
	//m1sdr_StartTimer();
	//(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	
	playing = true;
}


//
// m1sdr_PlayStart
// Stop the callback
//

void m1sdr_PlayStop(void)
{	
	// Stop playing
	playing = false;
	//(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
}


//
// m1sdr_FlushAudio
// Push all the sound currently in the buffer
//

void m1sdr_FlushAudio(void)
{
	m1sdr_PlayStart();
}


//
// m1sdr_SetSamplesPerTick
// Push all the sound currently in the buffer
//

void m1sdr_SetSamplesPerTick(UINT32 spf)
{
	int i;
	
	// samplerate/60
	nDSoundSegLen = spf;
	buffer = malloc(nDSoundSegLen * 2 * 2);
	memset(buffer, 0, nDSoundSegLen * 2 * 2);
	// these are the generator buffers
	/*for (i = 0; i < kMaxBuffers; i++)
	{
		if (buffer[i])
		{
			free(buffer[i]);
			buffer[i] = NULL;
		}

		buffer[i] = malloc(nDSoundSegLen * 2 * sizeof(UINT16));
		if (!buffer[i])
		{
			m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to allocate memory for sound buffer.", 0);
			//ExitToShell();
		}
		
		memset(buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
	}	

	// this is the playback buffer
	playBuffer = malloc(nDSoundSegLen * 2 * sizeof(UINT16));
	if (!playBuffer)
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to allocate memory for playback buffer.", 0);
		//ExitToShell();
	}
	
	memset(playBuffer, 0, nDSoundSegLen * 2 * sizeof(UINT16));

	//header->samplePtr = (char *)playBuffer;
	//header->numFrames = nDSoundSegLen;*/
}


//
// m1sdr_TimeCheck
// This function is not needed on the Mac
//

void m1sdr_TimeCheck(void)
{
}


//
// m1sdr_SetCallback
// Set the callback function
//

void m1sdr_SetCallback(void *fn)
{
	m1sdr_Callback = (void (*)(unsigned long, signed short *))fn;
}
