// null driver

#include "m1snd.h"

static void NULL_SendCmd(int cmda, int cmdb);

M1_BOARD_START( null )
	MDRV_NAME("Null skeleton driver")
	MDRV_HWDESC("No hardware")
	MDRV_SEND( NULL_SendCmd )
M1_BOARD_END

static void NULL_SendCmd(int cmda, int cmdb)
{
}
