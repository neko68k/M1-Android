#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include "m1sdr_android.h"
#include "thread_timer.h"

pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;

//#define WAIT_TIME_SECONDS       60
static int waitTime;
static int backupWait = 0;
static bool enabled = false;
static bool adjusted = false;

void *timerThreadFunc(void *ptr)
{
  int               rc;
  struct timespec   ts;
  struct timeval    tp;
  int *temp = (int*)ptr;

  waitTime = *temp;

  enabled = true;  

  /* Usually worker threads will loop on these operations */
  while (enabled) {
		rc = pthread_mutex_lock(&mutex);
		rc =  gettimeofday(&tp, NULL);
		//checkResults("gettimeofday()\n", rc);

		/* Convert from timeval to timespec */
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += (time_t)waitTime;

		//printf("Thread blocked\n");
		rc = pthread_cond_timedwait(&cond, &mutex, &ts);
		/* If the wait timed out, in this example, the work is complete, and   */
		/* the thread will end.                                                */
		/* In reality, a timeout must be accompanied by some sort of checking  */
		/* to see if the work is REALLY all complete. In the simple example    */
		/* we will just go belly up when we time out.                          */
		//if (rc == ETIMEDOUT) {
			rc = pthread_mutex_unlock(&mutex);
			// do what we need to do here!
			m1sdr_GenerationCallback();
			// if we did a one shot, set the wait time back where it should be
			if(adjusted)
				waitTime = backupWait;
		//}
	}  	
	rc = pthread_mutex_unlock(&mutex);
	return NULL;
}

void updateTimerWait(int newTime)
{
	backupWait = waitTime;
	waitTime = newTime;
	adjusted = true;
}

void stopTimer()
{
	enabled = false;
}


