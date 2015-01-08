/***************************************************************************

  timer.c

  CPU timing/synchro functions

***************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#define TIMER_MAX_CPUS	4
#define TIMER_MAX_TIMERS 32

typedef struct TimerT
{
	double time_to_fire;
	void (*callback)(int);
	double period;
	int loop;
	int refcon;
	int active;
	int enable;
} TimerT;

#define TIME_IN_HZ(hz)        (1.0 / (double)(hz))
#define TIME_IN_SEC(s)        ((double)(s))
#define TIME_IN_MSEC(ms)      ((double)(ms) * (1.0 / 1000.0))
#define TIME_IN_USEC(us)      ((double)(us) * (1.0 / 1000000.0))
#define TIME_IN_NSEC(us)      ((double)(us) * (1.0 / 1000000000.0))

#define TIME_TO_CYCLES(cpu, us)    ((double)(us) * timer_get_cpu_clock(cpu))

#define TIME_NOW              (0.0)
#define TIME_NEVER            (1.0e30)

#define TIMER_INACTIVE		(0)
#define TIMER_ACTIVE		(1)
#define TIMER_SPAWNED		(2)
#define TIMER_ALLOCED		(3)

void timer_init(long dsprate, double framerate);
void timer_reset(TimerT *tptr, double period);
void timer_clear_cpus(void);
int timer_add_cpu(int type, long clock, int(*runroutine)(int), int(*getcycles)(void), void(*yield)(void), void(*getcontext)(void *), void(*setcontext)(void *), void *ctx);
void timer_set_slices(int slices);
TimerT *timer_add(int loop, double period, void (*callback)(int), int refcon);
TimerT *timer_set(double period, int refcon, void (*callback)(int));
TimerT *timer_pulse(double period, int refcon, void (*callback)(int));
void timer_remove(TimerT *tptr);
void timer_run_frame(void);
void timer_yield(void);
double timer_get_cur_cpu_time(void);
double timer_get_cur_dsp_time(void);
double timer_timeelapsed(TimerT *tptr);
void timer_enable(TimerT *tptr, int enable);
void timer_setturbo(int factor);
void timer_setnoturbo(void);
void timer_clrnoturbo(void);
int timer_get_cur_cpu(void);
int timer_get_cpu_type(int cpu);
double timer_get_cpu_clock(int cpu);
void timer_save_cpu(int cpu);
void timer_restore_cpu(int cpu);
void timer_spin_cpu(int cpu);
void timer_unspin_cpu(int cpu);
void *timer_alloc(void (*callback)(int));
void timer_adjust(void *which, double duration, int param, double period);

#endif
