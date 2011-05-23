#ifndef __M1SDR_ANDROID_H__
#define __M1SDR_ANDROID_H__

#include "osddefs.h"
#include "oss.h"





extern void						(*m1sdr_Callback)(unsigned long dwUser, short *smp);
extern int nDSoundSegLen;
#ifdef __cplusplus
extern "C" {
#endif
jbyteArray m1sdrGenerationCallback(JNIEnv *env);
void waitForBoot();
#ifdef __cplusplus
}
#endif
/*
int m1sdr_GetPlayTime(void);
void m1sdr_GetPlayTimeStr(char *buf);
INT16 *m1sdr_GetPlayBufferPtr(void);
void m1sdr_InstallGenerationCallback(void);
void m1sdr_RemoveGenerationCallback(void);
char m1sdr_CurrentlyPlaying(void);*/


#endif
