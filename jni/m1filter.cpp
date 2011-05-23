/*
   m1filter.c - low-pass filter implementation, originally by The_Author

 */

#include <math.h>
#include "filter.h"
#include "m1filter.h"

static FILTWK	fwk[2];

static void FT_Q(FILTWK *p,long rate,long cutoff,double boost)
{
	double	total=(1<<FILT_BASE_BITS);
	double	r,fs,fc;
#ifndef IIR_FILTER
	long	i;
#endif

	fs=(double)rate;
	fc=(double)cutoff;
#ifndef IIR_FILTER
	//(CUTOFF*2)<=rate

	if((fc*2)>fs)	fc=fs/2;		//cutoff freq‚limitting
	total *= boost;
	for(i=0;i<FILT_TAPS;i++)
	{
		r=(2*PI*fc/fs)*((double)i-(double)(FILT_TAPS/2));
		if(r==0)
		{
			p->q[i]=(long)(2.0*total*fc/fs);
		}
		else
		{
			p->q[i]=(long)((2.0*total*fc/fs)*(sin(r)/r));
		}
	}
#else
	r=(2.0 * fc) / fs;
	if(r > 1) r=1;
	p->q[0]=(long)(r * total);			//coef
	p->q[1]=(long)((1.0-r) * total);	//feedback
#endif
}



#ifndef IIR_FILTER
#if 0
static long FT_DT(FILTWK *p,long dt)
{
	long	i, d, idx;

	p->dt[p->wt++]=dt;	
	if(p->wt >= FILT_TAPS)	p->wt=0;
	idx=p->rd;			
	for(i=0,d=0;i<FILT_TAPS;i++)		
	{
		d += p->dt[idx++] * p->q[i];		
		if(idx >= FILT_TAPS)	idx=0;		
	}						
	p->rd++;					
	if(p->rd >= FILT_TAPS)	p->rd=0;		
	return (d>>FILT_BASE_BITS);			
}
#endif
long filter_do0(long dt)
{
	long	i, d, idx;

	fwk[0].dt[fwk[0].wt++]=dt;	
	if(fwk[0].wt >= FILT_TAPS)	fwk[0].wt=0;
	idx=fwk[0].rd;			
	for(i=0,d=0;i<FILT_TAPS;i++)		
	{
		d += fwk[0].dt[idx++] * fwk[0].q[i];		
		if(idx >= FILT_TAPS)	idx=0;		
	}						
	fwk[0].rd++;					
	if(fwk[0].rd >= FILT_TAPS)	fwk[0].rd=0;		
	return (d>>FILT_BASE_BITS);			
}

long filter_do1(long dt)
{
	long	i, d, idx;

	fwk[1].dt[fwk[1].wt++]=dt;	
	if(fwk[1].wt >= FILT_TAPS)	fwk[1].wt=0;
	idx=fwk[1].rd;			
	for(i=0,d=0;i<FILT_TAPS;i++)		
	{
		d += fwk[1].dt[idx++] * fwk[1].q[i];		
		if(idx >= FILT_TAPS)	idx=0;		
	}						
	fwk[1].rd++;					
	if(fwk[1].rd >= FILT_TAPS)	fwk[1].rd=0;		
	return (d>>FILT_BASE_BITS);			
}
#else
static long FT_DT(FILTWK *p,long dt)
{
	p->dt[0]=(((dt * p->q[0]) + (p->dt[0] * p->q[1]))>>FILT_BASE_BITS);
	return p->dt[0];
}
static long FT_DT0(long dt)
{
	fwk[0].dt[0]=(((dt * fwk[0].q[0]) + (fwk[0].dt[0] * fwk[0].q[1]))>>FILT_BASE_BITS);
	return fwk[0].dt[0];
}
static long FT_DT1(long dt)
{
	fwk[1].dt[0]=(((dt * fwk[1].q[0]) + (fwk[1].dt[0] * fwk[1].q[1]))>>FILT_BASE_BITS);
	return fwk[1].dt[0];
}
#endif

void filter_init(int samplerate)
{
	int i;

	for(i=0 ; i < FILT_TAPS ; i++)
	{
		fwk[0].dt[i] = fwk[1].dt[i] = 0;
	}

	fwk[0].rd=0;
	fwk[1].rd=0;
	fwk[0].wt=FILT_TAPS-1;
	fwk[1].wt=FILT_TAPS-1;

	FT_Q(&fwk[0],samplerate,samplerate/2,1.0);
	FT_Q(&fwk[1],samplerate,samplerate/2,1.0);
}

