/*
 * timer.c - much less ambitious timing system than MAME's timer.c, but
 *           intended to be compatible anyway.
 *
 *	     Here are the base units for timing:
 *	     - Frames/second
 *           - Inside each frame time is determined by the CPU cycle
 *             count (accurate CPU emulation is a good thing here).
 *           - Real time is maintained by the sound card (ie, the 44100 Hz
 *             output stream is the final arbiter of real time).
 *
 *           This is similar to how MAME works.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "driver.h"
#include "timer.h"
#include "m1snd.h"

extern void update_dsps(long dspframes, long totalframes);

typedef struct CPUTimeT
{
	int type;			// cpu type tag for cpuintrf's benefit
	double clock;			// cpu clock in Hz
	double cycles_per_frame;	// # of cycles to run in a frame
	double cycles_per_slice;	// # of cycles to run in a slice
	double sec_to_cycles;		// divide time by this to get cycles
	double cycles_this_frame;	// cycles run this frame
	int active;			// 1 if active
	int (*emulroutine)(int);	// routine to run a CPU for x # of cycles
	int (*getcycles)(void);		// routine to get # of cycles elapsed 
	void (*yield)(void);		// routine to yield your timeslice
	void (*getcontext)(void *ctx);	// routine to copy the CPU's context into the "ctx" memory block
	void (*setcontext)(void *ctx);	// routine to copy the "ctx" memory block's context data to the CPU
	void *ctx;			// CPU's context
	int inuse;			// 1 if in use, 0 if not allocated
	int spin;			// in "spin" cycle (skips execution until unspun)
	unsigned int cycles_run_this_frame;
} CPUTimeT;

static double cur_frame_time;	// time spent in the current frame
static double cur_frame_left;	// time left in the frame
static double runtime;		// time to run current set of CPU slices
static int slices_per_frame;	// # of CPU slices per frame (for multi CPU systems)
static int cur_cpu;		// current running CPU
static double dsp_sec_to_cycles;
static double timer_frame_rate;	// frame time in Hz. (normally 60.0)

static CPUTimeT cpus[TIMER_MAX_CPUS];	
static TimerT tlist[TIMER_MAX_TIMERS];		// timer list

static int inside_update, no_turbo, last_cpu_add;

// get length to run everything for
static double timer_get_runtime(double timeleft)
{
	double ttr = timeleft;
	int i;

//	printf("get_runtime: start with %f\n", ttr);

	for (i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		if (tlist[i].active)
		{
			if (tlist[i].time_to_fire < ttr)
			{
//				printf("timer %d: Reducing TTR\n", i);
				ttr = tlist[i].time_to_fire;
			}
		}
	}

//	printf("get_runtime: end with %f\n", ttr);

	return(ttr);
}

// figure out how many cycles a CPU runs in a specific time
static long timer_comp_cycles(int cpu, double time)
{
	double cycles;

	cycles = cpus[cpu].clock*time;

//	printf("cycles = %f (time = %f)\n", cycles, time);

	return ((long)cycles);
}

// update all timers
static void timer_update_all_timers(double updtime)
{
	int i;
	TimerT *tptr;

	inside_update = 0;

//	printf("start timer_update_all_timers (%f)\n", updtime);
	for (i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		tptr = &tlist[i];

		if (tptr->active == TIMER_ACTIVE)
		{
			tptr->time_to_fire -= updtime;
//			printf("timer %d: updtime %f, ttf = %f\n", i, updtime, tptr->time_to_fire);
			if (tptr->time_to_fire <= 0.0)
			{
//				printf("callback for timer %d = %x, refcon %d\n", i, (unsigned int)tptr->callback, tptr->refcon);
				if (tptr->callback)
				{
					tptr->callback(tptr->refcon);
				}
//				printf("callback over\n");

				// deactivate the timer now, or loop it
				if (!tptr->loop)
				{
//					printf("removing timer %d (ptr %x)\n", i, (unsigned int)tptr);
					tptr->active = TIMER_INACTIVE;
				}
				else
				{
					// looping timer, make it so
//					printf("looping: new period %f\n", tptr->period);
					if (tptr->period <= 0.0) 
					{
//						printf("new period zero, setting to NEVER\n");
						tptr->period = TIME_NEVER;
					}
					tptr->time_to_fire = tptr->period;
				}
			}

		}
	}

	inside_update = 1;

//	printf("end timer_update_all_timers\n");
}

static int verbose = 0;

// run a single frame (1/60th of a second)
void timer_run_frame(void)
{
	int i;
	long full_cycles;

	cur_frame_time = (double)0.0;
	cur_frame_left = (double)(1.0/timer_frame_rate);

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		if (cpus[i].inuse)
		{
			cpus[i].cycles_this_frame = cur_frame_left;
			cpus[i].cycles_run_this_frame = 0;
		}
	}

//	if (1) printf("start frame -----\n");

	while (cur_frame_left > 0.0)
	{
		runtime = timer_get_runtime(cur_frame_left);

//		printf("runtime %f  frame_left %f\n", runtime, cur_frame_left);

		// run each CPU		
		if (runtime > 0.0)
		{
			for (i = 0; i < TIMER_MAX_CPUS; i++)
			{
				if (cpus[i].inuse)
				{
					// set the current cpu and context switch to it
					cur_cpu = i;
					timer_restore_cpu(i);

					// run the cpu
					full_cycles = timer_comp_cycles(i, runtime);
					if (full_cycles > 0)
					{
		  				if (verbose) printf("cpu %d for %d cycles (time left = %f / %d)\n", i, (int)full_cycles, cur_frame_left, (int)((double)cpus[i].clock*cur_frame_left));
						if (!cpus[i].spin)
						{
//							printf("run cpu %d for %d cycles (time left = %f / %d)\n", i, (int)full_cycles, cur_frame_left, (int)((double)cpus[i].clock*cur_frame_left)); 
			 				cpus[i].emulroutine(full_cycles);

							// check if slice ended early (if late we'll eat it)
							if (cpus[i].getcycles() < full_cycles)
							{
								cpus[i].cycles_run_this_frame += cpus[i].getcycles();

								// adjust for true runtime
								runtime = cpus[i].sec_to_cycles * (double)cpus[i].getcycles();
//								printf("slice actually took %d (out of %d) cycles (%f sec, %f Fs)\n", cpus[i].getcycles(), (int)full_cycles, runtime, 1.0/44100.0);
							}
						}
						else
						{
//							printf("cpu %d spin %d cycles (time left = %f)\n", i, (int)full_cycles, cur_frame_left); 
							cpus[i].cycles_run_this_frame += full_cycles;
						}
					}

					// get the current context
					timer_save_cpu(i);
				}
			}
		}
		cur_cpu = -1;

//		printf("final runtime = %f\n", runtime);

		// update timers
		timer_update_all_timers(runtime);

		// activate any newly-spawned timers
		for (i = 0; i < TIMER_MAX_TIMERS; i++)
		{
			if (tlist[i].active == TIMER_SPAWNED)
			{
				tlist[i].active = TIMER_ACTIVE;
			}
		}
		

		// update the time pointers
		cur_frame_time += runtime;
		cur_frame_left -= runtime;

		// update DSPs
		m1snd_update_dsps((long)(cur_frame_time/dsp_sec_to_cycles), Machine->sample_rate/timer_frame_rate);
	}

	#if 1
	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		if (cpus[i].inuse)
		{
//			printf("CPU %d: %d / %f cycles this frame\n", i, cpus[i].cycles_run_this_frame, cpus[i].cycles_per_frame);

			#if 0
			if ((!cpus[i].spin) && (cpus[i].cycles_per_frame > cpus[i].cycles_run_this_frame))
			{
				printf("makeup cycles\n");
 				cpus[i].emulroutine(cpus[i].cycles_per_frame - cpus[i].cycles_run_this_frame);
			}
			#endif
		}
	}
	#endif
	
//	if (1) printf("end frame -----\n");
}

/********************************************************************************/
/* Public functions */
/********************************************************************************/

// inits the timing system
void timer_init(long dsprate, double framerate)
{
	int i;

	slices_per_frame = 1;
	cur_frame_time = 0.0;
	cur_frame_left = 0.0;
	cur_cpu = -1;
	dsp_sec_to_cycles = 1.0/(double)dsprate;

	inside_update = 0;
	timer_frame_rate = framerate;

	// mark all CPUs deallocated
	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		cpus[i].active = 0;
		cpus[i].inuse = 0;
	}

	// also deactivate all timers
	for (i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		tlist[i].active = TIMER_INACTIVE;
		tlist[i].callback = NULL;
		tlist[i].period = 0.0;
	}
}

// clears allocated CPUs
void timer_clear_cpus(void)
{
	int i;

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		if (cpus[i].ctx != NULL)
		{
			free(cpus[i].ctx);
			cpus[i].ctx = NULL;
		}

		cpus[i].inuse = 0;
	}
}

// change a timer's time
void timer_reset(TimerT *tptr, double period)
{
	if (!tptr) return;

	tptr->time_to_fire = tptr->period = period;
}

// returns the last CPU # added
int timer_get_last_added_cpu(void)
{
	return last_cpu_add;
}

// adds a CPU
int timer_add_cpu(int type, long clock, int(*runroutine)(int), int(*getcycles)(void), void(*yield)(void), void(*getcontext)(void *), void(*setcontext)(void *), void *ctx)
{
	int cpu;
	int found = 0;

	// find a cpu to use
	for (cpu = 0; cpu < TIMER_MAX_CPUS; cpu++)
	{
		if (!cpus[cpu].inuse)
		{
			found = 1;
			break;
		}
	}

	assert(found);

	memset(&cpus[cpu], 0, sizeof(CPUTimeT));

	cpus[cpu].type = type;
	cpus[cpu].clock = (double)clock;
	cpus[cpu].emulroutine = runroutine;
	cpus[cpu].getcycles = getcycles;
	cpus[cpu].yield = yield;
	cpus[cpu].getcontext = getcontext;
	cpus[cpu].setcontext = setcontext;
	cpus[cpu].ctx = ctx;

	cpus[cpu].cycles_per_frame = clock/(double)60.0;	// frames per second
	cpus[cpu].cycles_per_slice = cpus[cpu].cycles_per_frame / (double)slices_per_frame;
	cpus[cpu].sec_to_cycles = 1.0/clock;
	cpus[cpu].inuse = 1;

//	printf("Add cpu #%d\n", cpu);
//	printf("clock %ld, cycles/frame %f, cycles/slice %f\n", clock, cpus[cpu].cycles_per_frame, cpus[cpu].cycles_per_frame / (double)slices_per_frame);
//	printf("STC = %f\n", cpus[cpu].sec_to_cycles);

	last_cpu_add = cpu;

	return cpu;
}

// sets CPU time slices per frame
void timer_set_slices(int slices)
{
	int i;

	slices_per_frame = slices;

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		cpus[i].cycles_per_slice = cpus[i].cycles_per_frame / (double)slices_per_frame; 
	}
}

// get time inside current cpu
double timer_get_cur_cpu_time(void)
{
	double rtime;

	if (cur_cpu == -1)
	{
		return 0.0;
	}

	// get # of cycles inside current CPU
	rtime = (double)cpus[cur_cpu].getcycles();
	rtime *= cpus[cur_cpu].sec_to_cycles;	// convert to seconds

	return(rtime);
}

// get time in samples inside current slice
double timer_get_cur_dsp_time(void)
{
	double rtime;

	if (cur_cpu == -1)
	{
		return 0.0;
	}

	// get # of cycles inside current CPU
	rtime = (double)cpus[cur_cpu].getcycles();
	rtime *= cpus[cur_cpu].sec_to_cycles;	// convert to seconds
	rtime /= dsp_sec_to_cycles;
//	printf("%d cycles, %f dsp frames\n", cpus[cur_cpu].getcycles(), rtime);

	return(rtime);
}

// returns time used (not 100% accurate)
double timer_timeelapsed(TimerT *tptr)
{
	if (!tptr) return 0.0;

	return (tptr->period - tptr->time_to_fire);
}

// add a timer to the list
TimerT *timer_add(int loop, double period, void (*callback)(int), int refcon)
{
	int i, found = 0;
	TimerT *newnode;

	for (i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		if (tlist[i].active == TIMER_INACTIVE)
		{
			found = 1;
			break;
		}
	}
	
	if (!found)
	{
		printf("out of timers!\n");
		for (i = 0; i < TIMER_MAX_TIMERS; i++)
		{
			printf("timer %d, time to fire %f, refcon %d\n", i, tlist[i].time_to_fire, tlist[i].refcon);
		}
//		exit(-1);
	}

//	printf("timer_add %d: loop %d period %f refcon %d\n", i, loop, period, refcon);

	newnode = &tlist[i];

	// if the timer was created inside the callback, it's effectively at time 0 in the next frame
	// so activate it right away.  otherwise, wait a tick :)
	// (I know this seems wrong, but mame ym2151.c is right for both raiden II and sega games this way)
	if (inside_update)
	{
		newnode->active = TIMER_ACTIVE;
	}
	else
	{
		newnode->active = TIMER_SPAWNED;
	}
	
	newnode->loop = loop;
	newnode->time_to_fire = newnode->period = period;
	newnode->callback = callback;
	newnode->refcon = refcon;

	// adjust first "fire time" by current time
	newnode->time_to_fire += timer_get_cur_cpu_time();

	// if this timer expires before the end of this period...
	if (cur_cpu != -1)
	{
		if (period < runtime)
		{
			cpus[cur_cpu].yield();
		}
	}

//	printf("timer_add: newnode at %x (#%d)\n", (unsigned long)newnode, i);
	return(newnode);
}

// allocate a timer (modern MAME API)
void *timer_alloc(void (*callback)(int))
{
	return timer_add(1, TIME_NEVER, callback, 0);
}

void timer_adjust(void *which, double duration, int param, double period)
{
	TimerT *timer = (TimerT *)which;

	timer->time_to_fire = duration;
	timer->period = period;
	timer->refcon = param;

	// adjust "fire time" by current time
	timer->time_to_fire += timer_get_cur_cpu_time();

	// if this timer expires before the end of this period...
	if (cur_cpu != -1)
	{
		if (period < runtime)
		{
			cpus[cur_cpu].yield();
		}
	}
}

// yield the current cpu timeslice
void timer_yield(void)
{
//	printf("yield cpu %d\n", cur_cpu);
	if (cur_cpu != -1)
	{
		cpus[cur_cpu].yield();
	}
}

// sets a timer's enable status
void timer_enable(TimerT *tptr, int enable)
{
	tptr->enable = enable;
}

// one-shot timer add
TimerT *timer_set(double period, int refcon, void (*callback)(int))
{
	return timer_add(0, period, callback, refcon);
}

// looping timer add
TimerT *timer_pulse(double period, int refcon, void (*callback)(int))
{
	return timer_add(1, period, callback, refcon);
}

// delete a timer
void timer_remove(TimerT *tptr)
{
	int i;

//	printf("timer_remove: %x\n", (unsigned long)tptr);

	// find the previous node
	for (i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		if (&tlist[i] == tptr)
		{
			// got it, mark it inactive and go on
//			printf("found, timer %d, deactivating\n", i);
			tlist[i].active = 0;
			break;
		}
	}
}

// returns which CPU is active
int timer_get_cur_cpu(void)
{
	return cur_cpu;
}

// sets a CPU to "spin"
void timer_spin_cpu(int cpu)
{
	cpus[cpu].spin = 1;
}

// sets a CPU back to normal
void timer_unspin_cpu(int cpu)
{
	cpus[cpu].spin = 0;

	if (cur_cpu == cpu)
	{
		cpus[cur_cpu].yield();
	}
}

// saves a CPU's context
void timer_save_cpu(int cpu)
{
	cpus[cpu].getcontext(cpus[cpu].ctx);
}

// restores a CPU's context
void timer_restore_cpu(int cpu)
{
	cpus[cpu].setcontext(cpus[cpu].ctx);
	memory_context_switch(cpu);
}

// set a "turbo factor" to temporarily boost the effective MHz of the CPUs
void timer_setturbo(int factor)
{
	int i;
	double clock;

	if (no_turbo) 
	{
		return;
	}

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		clock = cpus[i].clock;
		clock *= (double)factor;

		cpus[i].cycles_per_frame = clock/(double)60.0;	// frames per second
		cpus[i].cycles_per_slice = cpus[i].cycles_per_frame / (double)slices_per_frame;
		cpus[i].sec_to_cycles = 1.0/(double)clock;
	}
}

void timer_setnoturbo(void)
{
	no_turbo = 1;
}

void timer_clrnoturbo(void)
{
	no_turbo = 0;
}

int timer_get_cpu_type(int cpunum)
{
	return cpus[cpunum].type;
}

double timer_get_cpu_clock(int cpunum)
{
	return cpus[cpunum].clock;
}
