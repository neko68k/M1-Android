/***************************************************************************

	CDRDAO TOC parser for CHD compression frontend

***************************************************************************/

#ifndef _MAME_CHDCD_H_
#define _MAME_CHDCD_H_

int cdrom_parse_toc(char *tocfname, struct cdrom_toc *outtoc, struct cdrom_track_input_info *outinfo);

#endif
