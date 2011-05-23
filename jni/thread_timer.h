#ifndef __THREAD_TIMER_H__
#define __THREAD_TIMER_H__

void *timerThreadFunc(void *ptr);
void updateTimerWait(int newTime);
void stopTimer();

#endif
