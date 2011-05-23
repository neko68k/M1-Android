#ifndef _C352_H_
#define _C352_H_

struct C352interface 
{
	int mixing_level;			/* volume (front) */
	int mixing_level2;			/* volume (rear) */
	int region;				/* memory region of sample ROMs */
	int clock;
};

int c352_sh_start( const struct MachineSound *msound );
void c352_sh_stop( void );

READ16_HANDLER( c352_0_r );
WRITE16_HANDLER( c352_0_w );

#endif

