/**********************************************************************************************
 *
 *   DMA-driven DAC driver
 *   by Aaron Giles
 *
 **********************************************************************************************/

#ifndef DMADAC_H
#define DMADAC_H

#define MAX_DMADAC_CHANNELS 8

struct dmadac_interface
{
	int num;
	int mixing_level[MAX_DMADAC_CHANNELS];
};

int dmadac_sh_start(const struct MachineSound *msound);
void dmadac_transfer(UINT8 first_channel, UINT8 num_channels, offs_t channel_spacing, offs_t frame_spacing, offs_t total_frames, INT16 *data);
void dmadac_enable(UINT8 first_channel, UINT8 num_channels, UINT8 enable);
void dmadac_set_frequency(UINT8 first_channel, UINT8 num_channels, double frequency);
void dmadac_set_volume(UINT8 first_channel, UINT8 num_channels, UINT16 volume);

#endif

