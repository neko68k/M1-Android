#ifndef SAMPLES_H_
#define SAMPLES_H_

#define MAX_SAMPLES	(128)

void samples_init(int num, long sample_rate);
void samples_set_info(int chan, int playrate, void *data, int length);
void samples_play_chan(int chan);
void samples_stop_chan(int chan);

#endif
