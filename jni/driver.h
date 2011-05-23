#ifndef DRIVER_H
#define DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "rom.h"

#ifdef __GNU__
#define UNUSEDARG __attribute__((__unused__))
#else
#define UNUSEDARG
#endif

void logerror(char *fmt,...);

// MAME compatibility
#define memory_region(x) rom_getregion(x)
#define memory_region_length(x) rom_getregionsize(x)

struct cpu_info
{
	int CPU_type;
	long speed;
	void *memhandler;
	void *extra1;
	void *extra2;
};

struct M1Driver2
{
	int frames_per_second;
	struct MachineSound *sound;
	struct cpu_info *cpu;
};

struct GameSample
{
	int length;
	int smpfreq;
	int resolution;
	signed char data[1];	/* extendable */
};

struct GameSamples
{
	int total;	/* total number of samples */
	struct GameSample *sample[1];	/* extendable */
};

struct M1Machine
{
	long sample_rate;
	int refcon;	// m1-specific value

	struct M1Driver2 *drv;
	struct GameSamples *samples;
};

extern struct M1Machine *Machine;

#define ACCESSING_LSB ((mem_mask & 0x00ff) == 0)
#define ACCESSING_MSB ((mem_mask & 0xff00) == 0)

#define MAX_CPU 8	/* MAX_CPU is the maximum number of CPUs which cpuintrf.c */
					/* can run at the same time. Currently, 8 is enough. */

#define MAX_SOUND 5	/* MAX_SOUND is the maximum number of sound subsystems */
					/* which can run at the same time. Currently, 5 is enough. */




#include "cpuintrf.h"	// heck, why not?
#include "sndintrf.h"

// disable profiling
#define profiler_mark(x)

#define TIME_IN_HZ(hz)        (1.0 / (double)(hz))   

#ifdef __cplusplus
}
#endif
#endif
