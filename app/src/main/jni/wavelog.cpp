/*
  * wavelog.c: .WAV file logger
  *
  * Portions borrowed from FinalBurn "wave.cpp" and reworked
  *
  */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m1snd.h"
#include "m1ui.h"
#include "oss.h"
#include "wavelog.h"

static int waveLogEnable;
static FILE *waveLogFile = NULL;

static char wlpath[8192];

// init wave logging
void waveLogInit(int enable, char *name, int number)
{
//	char fname[256];

	waveLogEnable = enable;

	waveLogFile = (FILE *)NULL;

	// get the pathname
	strcpy(wlpath, "");
	m1ui_message(m1ui_this, M1_MSG_GETWAVPATH, wlpath, 0);

//	sprintf(fname, "%s%04d.wav", name, number);
//	strcat(wlpath, fname);
}

// external wave path set
void waveLogSetPath(char *newpath)
{
	strcpy(wlpath, newpath);
}

// begin wave logging
void waveLogStart(void)
{
	INT32 ua = 0;
	INT16 wFormatTag = 0;	// format 1 is linear PCM
	INT16 nChannels = 0;
	INT32 nSamplesPerSec = 0, nAvgBytesPerSec = 0;
	INT16 nBlockAlign = 0, wBitsPerSample = 0;

	// do not create file if disabled
	if (waveLogEnable == 0)
	{
		return;
	}

	waveLogStop();	// force end of any old log

	waveLogFile = fopen(wlpath, "wb");
	if (waveLogFile == (FILE*)NULL)
	{
		fprintf(stderr, "Unable to create .WAV log %s", wlpath);
		return;
	}

	// create the .WAV header
	nChannels = Endian16_Swap(2);
	nSamplesPerSec = Endian32_Swap(Machine->sample_rate);
	nAvgBytesPerSec = Endian32_Swap((Machine->sample_rate*4));
	wBitsPerSample = Endian16_Swap(16);
	nBlockAlign = Endian16_Swap((nChannels * (wBitsPerSample/8)));
	ua = Endian32_Swap(0x10);
	wFormatTag = Endian16_Swap(1);
	
	fwrite("RIFF    WAVEfmt ", 1, 0x10, waveLogFile);
	fwrite(&ua, 4, 1, waveLogFile);
	fwrite(&wFormatTag, sizeof(wFormatTag), 1, waveLogFile);
	fwrite(&nChannels, sizeof(nChannels), 1, waveLogFile);
	fwrite(&nSamplesPerSec, sizeof(nSamplesPerSec), 1, waveLogFile);
	fwrite(&nAvgBytesPerSec, sizeof(nAvgBytesPerSec), 1, waveLogFile);
	fwrite(&nBlockAlign, sizeof(nBlockAlign), 1, waveLogFile);
	fwrite(&wBitsPerSample, sizeof(wBitsPerSample), 1, waveLogFile);
	fwrite("data    ", 8, 1, waveLogFile);
}

// called per-frame to log wave data
void waveLogFrame(unsigned char *data, long bytes)
{
	if (waveLogFile != (FILE *)NULL)
	{
		int i;
		short *buf = (short *)data;
		for (i = 0; i < bytes >> 1; i++)
		{
			short val = Endian16_Swap(buf[i]);
			fwrite(&val, 2, 1, waveLogFile);
		}
		//fwrite(data, bytes, 1, waveLogFile);
	}
}

// ends logging
void waveLogStop(void)
{
	unsigned int nLen = 0, temp;

	if (waveLogEnable == 0)
	{
		return;
	}

	if (waveLogFile != (FILE *)NULL)
	{
		// get total file length
		fseek(waveLogFile, 0, SEEK_END);
		nLen = ftell(waveLogFile);

		// set RIFF chunk size (total minus 8 for the RIFF and length)
		fseek(waveLogFile, 4, SEEK_SET);
		nLen -= 8;
		temp = Endian32_Swap(nLen);
		fwrite(&temp, 4, 1, waveLogFile);

		// set WAVE chunk size (total - 8 - 36 more for the WAVE header stuff)
		fseek(waveLogFile, 0x28, SEEK_SET);
		nLen -= 36;
		temp = Endian32_Swap(nLen);
		fwrite(&temp, 4, 1, waveLogFile);

		fclose(waveLogFile);
		waveLogFile = (FILE *)NULL;
	}
}

