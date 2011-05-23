#ifndef _WAVELOG_H_
#define _WAVELOG_H_

void waveLogInit(int enable, char *name, int number);
void waveLogSetPath(char *newpath);
void waveLogStart(void);
void waveLogFrame(unsigned char *data, long bytes);
void waveLogStop(void);

#endif

