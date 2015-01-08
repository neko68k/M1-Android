/***************************************************************************

  2612intf.c

  The YM2612 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include "driver.h"
#include "../sound/fm.h"
#include "../sound/2612intf.h"


#ifdef BUILD_YM2612

#define YM2612_NUMBUF 2

/* use FM.C with stream system */

static int stream[MAX_2612];
static TimerT *timers[MAX_2612][2];

/* Global Interface holder */
static const struct YM2612interface *intf;

//static void *Timer[MAX_2612][2];
//static double lastfired[MAX_2612][2];

/*------------------------- TM2612 -------------------------------*/
/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if(intf->handler[n]) intf->handler[n](irq);
}

/* Timer overflow callback from timer.c */
static void ym2612_callback(int refcon)
{
	int n, c, ref;

	ref = (int)refcon;
	n = ref>>8;
	c = ref&0xf;

//	printf("timer expired, chip %d timer %d\n", n, c);

	YM2612TimerOver(n, c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,int cnt,double stepTime)
{
	int refcon;

//	printf("YM_Timer: chip=%d, timer=%d, count=%d, step=%f\n", n, c, cnt, stepTime);

	if (cnt == 0)
	{
//		printf("reset YM timer!\n");
		timer_remove(timers[n][c]);
	}
	else
	{
		double timeSec = (double)cnt * stepTime;

//		printf("setting timer for %f sec\n", timeSec);

		refcon = (n<<8)|c;
		timers[n][c] = timer_add(0, timeSec, ym2612_callback, refcon);
	}
}

static void FMTimerInit( void )
{
}

/* update request from fm.c */
void YM2612UpdateRequest(int chip)
{
	stream_update(stream[chip], 0);
}

/***********************************************************/
/*    YM2612 (fm4ch type)                                  */
/***********************************************************/
int YM2612_sh_start(const struct MachineSound *msound)
{
	int i,j;
	char buf[YM2612_NUMBUF][40];
	const char *name[YM2612_NUMBUF];

	intf = msound->sound_interface;
	if( intf->num > MAX_2612 ) return 1;

	/* FM init */
	/* Timer Handler set */
	FMTimerInit();
	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		int vol[YM2612_NUMBUF];
		/* stream setup */
		for (j = 0 ; j < YM2612_NUMBUF ; j++)
		{
			vol[j] = intf->mixing_level[j];
			name[j] = buf[j];
			sprintf(buf[j],"YM2612(%s) #%d",j < 2 ? "FM" : "ADPCM",i);
		}
		stream[i] = stream_init_multi(YM2612_NUMBUF,
			name,vol,intf->baseclock/144,
			i,YM2612UpdateOne);
	}

	/**** initialize YM2612 ****/
	if (YM2612Init(intf->num,intf->baseclock,intf->baseclock/144,TimerHandler,IRQHandler) == 0)
	  return 0;
	/* error */
	return 1;
}

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void YM2612_sh_stop(void)
{
	YM2612Shutdown();
}

/* reset */
void YM2612_sh_reset(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
		YM2612ResetChip(i);
}

/************************************************/
/* Status Read for YM2612 - Chip 0				*/
/************************************************/
READ_HANDLER( YM2612_status_port_0_A_r )
{
  return YM2612Read(0,0);
}

READ_HANDLER( YM2612_status_port_0_B_r )
{
  return YM2612Read(0,2);
}

/************************************************/
/* Status Read for YM2612 - Chip 1				*/
/************************************************/
READ_HANDLER( YM2612_status_port_1_A_r ) {
  return YM2612Read(1,0);
}

READ_HANDLER( YM2612_status_port_1_B_r ) {
  return YM2612Read(1,2);
}

/************************************************/
/* Port Read for YM2612 - Chip 0				*/
/************************************************/
READ_HANDLER( YM2612_read_port_0_r ){
  return YM2612Read(0,1);
}

/************************************************/
/* Port Read for YM2612 - Chip 1				*/
/************************************************/
READ_HANDLER( YM2612_read_port_1_r ){
  return YM2612Read(1,1);
}

/************************************************/
/* Control Write for YM2612 - Chip 0			*/
/* Consists of 2 addresses						*/
/************************************************/
WRITE_HANDLER( YM2612_control_port_0_A_w )
{
  YM2612Write(0,0,data);
}

WRITE_HANDLER( YM2612_control_port_0_B_w )
{
  YM2612Write(0,2,data);
}

/************************************************/
/* Control Write for YM2612 - Chip 1			*/
/* Consists of 2 addresses						*/
/************************************************/
WRITE_HANDLER( YM2612_control_port_1_A_w ){
  YM2612Write(1,0,data);
}

WRITE_HANDLER( YM2612_control_port_1_B_w ){
  YM2612Write(1,2,data);
}

/************************************************/
/* Data Write for YM2612 - Chip 0				*/
/* Consists of 2 addresses						*/
/************************************************/
WRITE_HANDLER( YM2612_data_port_0_A_w )
{
  YM2612Write(0,1,data);
}

WRITE_HANDLER( YM2612_data_port_0_B_w )
{
  YM2612Write(0,3,data);
}

/************************************************/
/* Data Write for YM2612 - Chip 1				*/
/* Consists of 2 addresses						*/
/************************************************/
WRITE_HANDLER( YM2612_data_port_1_A_w ){
  YM2612Write(1,1,data);
}
WRITE_HANDLER( YM2612_data_port_1_B_w ){
  YM2612Write(1,3,data);
}

/**************** end of file ****************/

#endif
