/***************************************************************************

	Generic MAME cd-rom implementation

***************************************************************************/

#ifndef _MAME_CDROM_H_
#define _MAME_CDROM_H_

#include "driver.h"
#include "chd.h"

struct cdrom_file;

#define CD_MAX_TRACKS		(99)	/* AFAIK the theoretical limit */
#define CD_MAX_SECTOR_DATA	(2352)
#define CD_MAX_SUBCODE_DATA	(96)

#define CD_FRAME_SIZE		(CD_MAX_SECTOR_DATA + CD_MAX_SUBCODE_DATA)
#define CD_FRAMES_PER_HUNK	(4)

#define CD_METADATA_WORDS	(1+(CD_MAX_TRACKS * 6))
enum
{
	CD_TRACK_MODE1 = 0, 	/* mode 1 2048 bytes/sector */
	CD_TRACK_MODE1_RAW,	/* mode 1 2352 bytes/sector */
	CD_TRACK_MODE2,		/* mode 2 2336 bytes/sector */
	CD_TRACK_MODE2_FORM1,	/* mode 2 2048 bytes/sector */
	CD_TRACK_MODE2_FORM2,	/* mode 2 2324 bytes/sector */
	CD_TRACK_MODE2_FORM_MIX, /* mode 2 2336 bytes/sector */
	CD_TRACK_MODE2_RAW,	/* mode 2 2352 bytes / sector */
	CD_TRACK_AUDIO,		/* redbook audio track 2352 bytes/sector (588 samples) */
};

enum
{
	CD_SUB_NORMAL = 0, 	/* "cooked" 96 bytes per sector */
	CD_SUB_RAW,		/* raw uninterleaved 96 bytes per sector */
	CD_SUB_NONE,		/* no subcode data stored */
};

struct cdrom_track_info
{
	/* fields used by CHDMAN and in MAME */
	UINT32 trktype;		/* track type */
	UINT32 subtype;		/* subcode data type */
	UINT32 datasize;	/* size of data in each sector of this track */
	UINT32 subsize;		/* size of subchannel data in each sector of this track */
	UINT32 frames;		/* number of frames in this track */
	UINT32 extraframes;	/* number of "spillage" frames in this track */

	/* fields used in MAME only */	
	UINT32 physframeofs;	/* frame number on the real CD this track starts at */
	UINT32 chdframeofs;	/* frame number this track starts at on the CHD */
};

struct cdrom_track_input_info	/* used only at compression time */
{
	char fname[CD_MAX_TRACKS][256];	/* filename for each track */
	UINT32 offset[CD_MAX_TRACKS];	/* offset in the data file for each track */
};

struct cdrom_toc
{
	UINT32 numtrks;		/* number of tracks */
	struct cdrom_track_info tracks[CD_MAX_TRACKS];
};

/* base functionality */
struct cdrom_file *cdrom_open(struct chd_file *chd);
UINT32 cdrom_read_data(struct cdrom_file *file, UINT32 lbasector, UINT32 numsectors, void *buffer, UINT32 datatype);
void cdrom_close(struct cdrom_file *file);

/* handy utilities */
UINT32 cdrom_get_track_phys(struct cdrom_file *file, UINT32 frame);
UINT32 cdrom_get_phys_start_of_track(struct cdrom_file *file, UINT32 track);
UINT32 cdrom_get_chd_start_of_track(struct cdrom_file *file, UINT32 track);
UINT32 cdrom_phys_frame_to_chd(struct cdrom_file *file, UINT32 frame);
UINT32 cdrom_chd_frame_to_phys(struct cdrom_file *file, UINT32 frame);
struct chd_file *cdrom_get_chd(struct cdrom_file *file);

/* Red Book audio track utilities */
void cdrom_start_audio(struct cdrom_file *file, UINT32 start_chd_lba, UINT32 blocks);
void cdrom_stop_audio(struct cdrom_file *file);
void cdrom_pause_audio(struct cdrom_file *file, int pause);
UINT32 cdrom_get_audio_lba(struct cdrom_file *file);
int cdrom_audio_active(struct cdrom_file *file);
int cdrom_audio_paused(struct cdrom_file *file);
int cdrom_audio_ended(struct cdrom_file *file);

void cdrom_get_audio_data(struct cdrom_file *file, INT16 *bufL, INT16 *bufR, UINT32 bytes_wanted);

#endif	// _MAME_CDROM_H_

