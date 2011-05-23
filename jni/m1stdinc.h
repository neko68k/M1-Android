// a mini version of m1snd.h for files not needing a lot

#ifndef _M1STDINC_H_
#define _M1STDINC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "timer.h"
#include "board.h"

#include "driver.h"	// MAME includes
#include "cpuintrf.h"

#include "6821pia.h"
#include "taitosnd.h"

#include "m68k.h"	// cpu includes
#include "m6809.h"	
#include "i8039.h"
#include "i8085.h"
#include "m6502.h"
#include "h6280.h"
#include "m6800.h"
#include "necintrf.h"
#include "adsp2100.h"
#ifdef USE_DRZ80
	#include "drz80_z80.h"
#endif
#ifdef USE_Z80
	#include "z80.h"
#endif
#include "m37710.h"
#include "hd6309.h"
#include "h83002.h"
#include "tms32031.h"

#include "fm.h"		// sound includes
#include "multipcm.h"
#include "ym2151.h"
#include "segapcm.h"
#include "rf5c68.h"
#include "ay8910.h"
#include "fmopl.h"
#include "adpcm.h"
#include "scsp.h"
#include "qsound.h"
#include "k007232.h"
#include "k053260.h"
#include "k054539.h"
#include "c140.h"
#include "tms57002.h"
#include "upd7759.h"
#include "samples.h"
#include "dac.h"
#include "pokey.h"
#include "es5506.h"
#include "5220intf.h"
#include "msm5205.h"
#include "ymf278b.h"
#include "iremga20.h"
#include "hc55516.h"
#include "bsmt2000.h"
#include "k005289.h"
#include "namco.h"
#include "cem3394.h"
#include "ymz280b.h"
#include "vlm5030.h"
#include "ymf271.h"
#include "scsp.h"
#include "msm5232.h"

#ifdef __cplusplus
}
#endif

// MAME cheat

#endif
