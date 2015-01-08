/***************************************************************************
                          filter.h  -  description
                             -------------------
    begin                : Wed Dec 15 1999
 ***************************************************************************/

#ifndef _FILTER_H
#define _FILTER_H

#define FILT_TAPS		5
#define FILT_BASE_BITS	8	

//--Filter work struct
typedef struct
{
	long	q[FILT_TAPS];
	long	dt[FILT_TAPS];
	long	rd;	//read point
	long	wt;	//write point
} FILTWK;

#define	PI	3.14159265358979

#endif // _FILTER_H

