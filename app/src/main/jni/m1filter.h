#ifndef _M1FILTER_H_
#define _M1FILTER_H_

void filter_init(int samplerate);
long filter_do0(long dt);
long filter_do1(long dt);

#endif
