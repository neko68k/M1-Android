#ifndef _M1QUEUE_H_
#define _M1QUEUE_H_

// functions for drivers
void m1snd_addToCmdQueue(int cmd);
void m1snd_addToCmdQueueRaw(int cmd);
void m1snd_setCmdPrefix(int cmd);
void m1snd_setCmdPrefixStr(int *suffix);
void m1snd_setCmdSuffixStr(int *suffix);
void m1snd_setNoPrefixOnStop(void);

// "internal" functions for the core
void m1snd_initQueue(void);
void m1snd_processQueue(void);
void m1snd_startQueue(void);
int m1snd_isQueueBusy(void);
int m1snd_getQueueBootTime(void);
void m1snd_setQueueInitCmd(int cmd);

#endif
