/***************************************************************************

	CDRDAO TOC parser for CHD compression frontend

***************************************************************************/

#include "osd_cpu.h"
#include "driver.h"
#include "harddisk.h"
#include "cdrom.h"
#include "md5.h"
#include "sha1.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/***************************************************************************
	CONSTANTS & DEFINES
***************************************************************************/

#define EATWHITESPACE	\
	while ((linebuffer[i] == ' ' || linebuffer[i] == 0x09 || linebuffer[i] == '\r') && (i < 512))	\
	{	\
		i++;	\
	}

#define TOKENIZE	\
	j = 0; \
	while (!isspace(linebuffer[i]) && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZETOCOLON	\
	j = 0; \
	while ((linebuffer[i] != ':') && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

/***************************************************************************
	PROTOTYPES
***************************************************************************/

/***************************************************************************
	GLOBAL VARIABLES
***************************************************************************/

static char linebuffer[512];

/***************************************************************************
	IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
	cdrom_parse_toc - parse a CDRDAO format TOC file
-------------------------------------------------*/

int cdrom_parse_toc(char *tocfname, struct cdrom_toc *outtoc, struct cdrom_track_input_info *outinfo)
{
	FILE *infile;
	int i, j, k, trknum, m, s, f, foundcolon;
	static char token[128];

	infile = fopen(tocfname, "rt");
	
	if (infile == (FILE *)NULL)
	{
		return CHDERR_FILE_NOT_FOUND;
	}

	/* clear structures */
	memset(outtoc, 0, sizeof(struct cdrom_toc));
	memset(outinfo, 0, sizeof(struct cdrom_track_input_info));

	trknum = -1;

	while (!feof(infile))
	{
		/* get the next line */
		fgets(linebuffer, 511, infile);

		/* if EOF didn't hit, keep going */
		if (!feof(infile))
		{
			i = 0;
			EATWHITESPACE
			TOKENIZE
		
			if ((!strcmp(token, "DATAFILE")) || (!strcmp(token, "FILE")))
			{
				/* found the data file for a track */
				EATWHITESPACE
				TOKENIZE

				/* keep the filename */
				strncpy(&outinfo->fname[trknum][0], &token[1], strlen(&token[1])-1);

				/* get either the offset or the length */
				EATWHITESPACE

				if (linebuffer[i] == '#')
				{
					/* it's a decimal offset, use it */
					TOKENIZE
					outinfo->offset[trknum] = strtoul(&token[1], NULL, 10);

					/* we're using this token, go on */
					EATWHITESPACE
					TOKENIZETOCOLON
				}
				else	
				{
					/* no offset, just M:S:F */
					outinfo->offset[trknum] = 0;
					TOKENIZETOCOLON
				}

				/*
				   This is tricky: the next number can be either a raw
				   number or an M:S:F number.  Check which it is.
				   If a space occurs before a colon in the token,
				   it's a raw number.
				*/

				foundcolon = 0;
				for (k = 0; k < strlen(token); k++)
				{
					if (token[k] == ' ')
					{
						break;
					}
				}

				if (k == strlen(token))
				{
					foundcolon = 1;
				}

				if (!foundcolon)
				{
					// rewind to the start of the real MSF
					while (linebuffer[i] != ' ')
					{
						i--;
					}

					i++;

					f = strtoul(token, NULL, 10);
				}
				else
				{
					/* now get the MSF format length (might be MSF offset too) */
					m = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZETOCOLON
					s = strtoul(token, NULL, 10);
					i++;	/* skip the colon */ 
					TOKENIZE
					f = strtoul(token, NULL, 10);

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);
				}

				EATWHITESPACE
				if (isdigit(linebuffer[i]))
				{
					f *= outtoc->tracks[trknum].datasize;

					outinfo->offset[trknum] += f;

					EATWHITESPACE
					TOKENIZETOCOLON

					m = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZETOCOLON
					s = strtoul(token, NULL, 10);
					i++;	/* skip the colon */ 
					TOKENIZE
					f = strtoul(token, NULL, 10);

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);
				}

				outtoc->tracks[trknum].frames = f;
			}
			else if (!strcmp(token, "TRACK"))
			{
				/* found a new track */
				trknum++;

				/* next token on the line is the track type */
				EATWHITESPACE
				TOKENIZE

				if (!strcmp(token, "MODE1"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE1;
					outtoc->tracks[trknum].datasize = 2048;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE1_RAW"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE1_RAW;
					outtoc->tracks[trknum].datasize = 2352;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2;
					outtoc->tracks[trknum].datasize = 2336;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_FORM1"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_FORM1;
					outtoc->tracks[trknum].datasize = 2048;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_FORM2"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_FORM2;
					outtoc->tracks[trknum].datasize = 2324;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_FORM_MIX"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_FORM_MIX;
					outtoc->tracks[trknum].datasize = 2336;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_RAW"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_RAW;
					outtoc->tracks[trknum].datasize = 2352;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "AUDIO"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_AUDIO;
					outtoc->tracks[trknum].datasize = 2352;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else 
				{
					printf("ERROR: Unknown track type [%s].  Contact MAMEDEV.\n", token);
				}

				/* next (optional) token on the line is the subcode type */
				EATWHITESPACE
				TOKENIZE

				if (!strcmp(token, "RW"))
				{
					outtoc->tracks[trknum].subtype = CD_SUB_NORMAL;
					outtoc->tracks[trknum].subsize = 96;
				}
				else if (!strcmp(token, "RW_RAW"))
				{
					outtoc->tracks[trknum].subtype = CD_SUB_RAW;
					outtoc->tracks[trknum].subsize = 96;
				}
			}
		}
	}

	/* close the input TOC */
	fclose(infile);

	/* store the number of tracks found */
	outtoc->numtrks = trknum + 1;

	return CHDERR_NONE;
}
