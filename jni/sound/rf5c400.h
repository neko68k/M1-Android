/* Ricoh RF5C400 emulator */

#ifndef __RF5C400_H__
#define __RF5C400_H__

struct RF5C400interface {
	int region;						/* memory regions of sample ROM(s) */
};

int rf5c400_sh_start(const struct MachineSound *msound);

READ16_HANDLER( RF5C400_0_r );
WRITE16_HANDLER( RF5C400_0_w );

#endif
