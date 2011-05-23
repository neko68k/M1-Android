/*********************************************************/
/*    ricoh RF5C68(or clone) PCM controller              */
/*********************************************************/
#ifndef __RF5C68_H__
#define __RF5C68_H__

/******************************************/
WRITE8_HANDLER( RF5C68_reg_w );

READ8_HANDLER( RF5C68_r );
WRITE8_HANDLER( RF5C68_w );

struct RF5C68interface
{
	int clock;
	int volume;
};

int RF5C68_sh_start( const struct MachineSound *msound );
void RF5C68_sh_stop( void );


#endif
/**************** end of file ****************/
