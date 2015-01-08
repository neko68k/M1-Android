/*
  board.h - defines a "board", which is a game hardware configuration along
            with similar varients.  It's equivalent to a "driver" in MAME.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include "driver.h"
#include "sndintrf.h"

#define MAX_CUSTOM_TAGS	(8)

//
// game board driver definitions
//
// internal def, being phased out of direct driver use
typedef struct
{
	long startupTime;			// startup ticks
	long interCmdTime;			// time between commands
	void (*InitBoard)(long srate);		// initialize the board
	void (*UpdateDSPs)(long dsppos, long dspframes);	// update sound
	void (*RunFrame)();			// called each frame if exists
	void (*SendCmd)(int cmda, int cmdb); 	// execute a command
	void (*Shutdown)(void);			// shuts down the board

	char *bname;				// name of board
	char *bhw;				// board's hardware description

	struct MachineSound bmachine[8];	// board's hardware
	struct cpu_info bcpus[4];		// board's CPUs

	void (*StopCmd)(int cmda); 		// when the stop command needs code support
} BoardT;

typedef struct 
{
	char label[32];
	char value[32];
} BoardOptionsT;

typedef struct m1game_t
{
	char *name;
	char *year;
	char *parentzip;
	char *zipname;
	int mfg;
	int btype;
	int defcmd;
	int stopcmd;
	RomEntryT roms[ROM_MAX];
	int mincmd;
	int maxcmd;
	int refcon;
	INT16 def_normalization;
	char *mfgstr;
	int numcustomtags;
	BoardOptionsT custom_tags[MAX_CUSTOM_TAGS];
} M1GameT;

extern M1GameT *games;

// tag IDs
#define TID_INVALID	0

#define	TID_CPU		1
#define TID_CPURW	2
#define TID_CPUOPREAD	3
#define TID_CPUPORTRW	4
#define TID_CPUMEMHAND	5

#define TID_SOUND	10

#define	TID_DELAYS	20

#define TID_INIT	30
#define TID_UPDATE	31
#define TID_RUN		32
#define TID_SEND	33
#define TID_SHUTDOWN	34
#define TID_STOP	35

#define	TID_NAME	40
#define TID_HW		41

#define TID_ROMS	50

// new style version
typedef struct
{
	int	id;	// tag ID
	void *data0;
	void *data1;
	void *data2;
	void *data3;
} TagT;

#define MAX_TAGS	(16)

typedef struct
{
	char id[5];
	TagT tags[MAX_TAGS];
} DrivertagsT;

#define M1_BOARD_START(game)	\
	DrivertagsT brd_##game = {	\
		"TAG0",	{


#define M1_BOARD_END	\
	},	\
	};
			
#define MDRV_ROMS_ADD( game )	\
	{	TID_ROMS, (void *)roms_##game },

#define MDRV_CPU_ADD(type, clock)	\
	{	TID_CPU, (void *)CPU_##type, (void *)(clock)	},

#define MDRV_CPU_MEMORY(read, write)	\
	{	TID_CPURW, (void *)&read, (void *)&write },

#define MDRV_CPU_PORTS(read, write)	\
	{	TID_CPUPORTRW, (void *)&read, (void *)&write },

#define MDRV_CPU_READOP(read)	\
	{	TID_CPUOPREAD, (void *)&read },

#define MDRV_CPUMEMHAND(handler)	\
	{	TID_CPUMEMHAND,	(void *)handler	},

#define MDRV_SOUND_ADD(type, interface)	\
	{	TID_SOUND, (void *)SOUND_##type, (void *)interface },

#define MDRV_INIT(func) \
	{	TID_INIT, (void*)func },
#define MDRV_UPDATE(func) \
	{	TID_UPDATE, (void*)func },
#define MDRV_RUN(func) \
	{	TID_RUN, (void*)func },
#define MDRV_SEND(func) \
	{	TID_SEND, (void*)func },
#define MDRV_SHUTDOWN(func) \
	{	TID_SHUTDOWN, (void*)func },
#define MDRV_STOP(func) \
	{	TID_STOP, (void*)func },

#define MDRV_NAME(name)	\
	{	TID_NAME, (void *)name },
#define MDRV_HWDESC(name)	\
	{	TID_HW, (void *)name },

#define MDRV_DELAYS(boot, cmd)	\
	{	TID_DELAYS, (void*)(boot), (void*)(cmd)	},

// handy macro
#define DRIVER( name )	extern DrivertagsT brd_##name;

#include "drvlist.h"
#endif
