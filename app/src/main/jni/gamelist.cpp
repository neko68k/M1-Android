/* --------------------------------
 * gamelist.cpp - game list support
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "m1snd.h"
#include "m1ui.h"

#include "expat.h"

static char element_data[2048];
static int numgames, currgn;

typedef enum
{
	XML_INVALID = 0,
	XML_GOTM1,
	XML_GAME,
	XML_TITLE,
	XML_YEAR,
	XML_MAKER,
	XML_M1DATA,
	XML_REGION,
	XML_ROM,
	XML_COMPLETE,
} XMLStateE;

static XMLStateE xml_state;

static int rgn_from_name(char *name)
{
	static char *rnames[RGN_MAX] =
	{
		(char *)"cpu1", (char *)"cpu2", (char *)"cpu3", (char *)"cpu4", (char *)"samp1", (char *)"samp2", 
		(char *)"samp3", (char *)"samp4", (char *)"user1", (char *)"user2", (char *)"disk"
	};
	static int rnums[RGN_MAX] =
	{
		RGN_CPU1, RGN_CPU2, RGN_CPU3, RGN_CPU4, RGN_SAMP1, RGN_SAMP2, RGN_SAMP3, RGN_SAMP4, RGN_USER1, RGN_USER2, RGN_DISK
	};
	int i;

	for (i = 0; i < RGN_MAX; i++)
	{
		if (!strcmp(rnames[i], name))
		{
			return rnums[i];
		}
	}

	printf("ERROR: unable to find region [%s]\n", name);
	return 0;
}

static int find_board_by_name(char *name)
{
	int maxboard = m1snd_get_info_int(M1_IINF_MAXDRVS, 0);
	int i;

	for (i = 0; i < maxboard; i++)
	{
		if (!strcmp(m1snd_get_info_str(M1_SINF_BNAME, i), name))
		{
			return i;
		}
	}

	printf("ERROR: unable to find board [%s]\n", name);
	return 0;
}

static void startElement(void *userData, const char *name, const char **atts)
{
	int i, val;
	int *depthPtr = (int *)userData;
	char *eptr;

	element_data[0] = '\0';

	if (!strcmp(name, "m1"))		xml_state = XML_GOTM1;
	else if (!strcmp(name, "description"))	xml_state = XML_TITLE;
	else if (!strcmp(name, "year"))		xml_state = XML_YEAR;
	else if (!strcmp(name, "manufacturer"))	xml_state = XML_MAKER;
	else if (!strcmp(name, "m1data"))	
	{
		xml_state = XML_M1DATA;
		games[curgame].numcustomtags = 0;	
	}
	else if (!strcmp(name, "game"))
	{
		// bump up both the current game number and the number of games
		curgame++;
		numgames++;

		// (re)size the gamelist to fit the new number of games
		games = (M1GameT *)realloc(games, sizeof(M1GameT)*numgames);

		// zero out the newly allocated entry
		memset(&games[curgame], 0, sizeof(M1GameT));

		// parentzip must exist even if no element for it is present (for back compatibility),
		// so allocate it now
		games[curgame].parentzip = (char *)malloc(16);
		games[curgame].parentzip[0] = '\0';

		// and set the state
		xml_state = XML_GAME;

		// reset the region pointer too
		currgn = -1;
	}
	else if ((!strcmp(name, "rom")) || (!strcmp(name, "disk")))	// we'll accept both
	{
		xml_state = XML_ROM;

		if (currgn < (ROM_MAX-1))
		{
			currgn++; 
		}

		// clear our flags
		games[curgame].roms[currgn].flags = 0;

		// mark the end of the list now (the clear flags step above will take care of it if we get there)
		games[curgame].roms[currgn+1].flags = ROM_ENDLIST;
	}
	else if (!strcmp(name, "region"))	
	{ 
		xml_state = XML_REGION; 
		
		if (currgn < (ROM_MAX-1))
		{
			currgn++; 
		}

		// clear our flags
		games[curgame].roms[currgn].flags = 0;

		// mark the end of the list now (the clear flags step above will take care of it if we get there)
		games[curgame].roms[currgn+1].flags = ROM_ENDLIST;
	};

	i = 0;
	while (atts[i] != (const char *)NULL)
	{
//		printf("%s = [%s]\n", atts[i], atts[i+1]);
		switch (xml_state)
		{
			case XML_GOTM1:
				if (!strcmp(atts[i], "version"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					if (val != 1)
					{
						printf("Unknown M1.XML file version.  M1 may malfunction!\n");
					}
/*					else
					{
						printf("XML version OK\n");
					}*/
				}
				break;

			case XML_GAME:
				if (!strcmp(atts[i], "name"))
				{
					games[curgame].zipname = (char *)malloc(strlen(atts[i+1])+1);
					strcpy(games[curgame].zipname, atts[i+1]);
				}
				else if (!strcmp(atts[i], "board"))
				{
					games[curgame].btype = find_board_by_name((char *)atts[i+1]);
				}
				else if (!strcmp(atts[i], "romof"))
				{
					strcpy(games[curgame].parentzip, atts[i+1]);
				}
				break;

			case XML_M1DATA:
				if (!strcmp(atts[i], "default"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].defcmd = val;
				}
				else if (!strcmp(atts[i], "stop"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].stopcmd = val;
				}
				else if (!strcmp(atts[i], "min"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].mincmd = val;
				}
				else if (!strcmp(atts[i], "max"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].maxcmd = val;
				}
				else if (!strcmp(atts[i], "subtype"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].refcon = val;
				}
				else
				{
//					printf("custom tag %d: %s = %s\n", games[curgame].numcustomtags, atts[i], atts[i+1]);
					strncpy(games[curgame].custom_tags[games[curgame].numcustomtags].label, atts[i], 31);
					strncpy(games[curgame].custom_tags[games[curgame].numcustomtags].value, atts[i+1], 31);
					if (games[curgame].numcustomtags < MAX_CUSTOM_TAGS) 
					{
						games[curgame].numcustomtags++;
					}
					else
					{
						printf("Ran out of custom tags for game, only %d allowed\n", MAX_CUSTOM_TAGS);
					}
				}
				break;

			case XML_REGION:
				if (!strcmp(atts[i], "type"))
				{
					games[curgame].roms[currgn].loadadr = rgn_from_name((char *)atts[i+1]);
					games[curgame].roms[currgn].flags |= ROM_RGNDEF;
//					printf("ROM %d: REGION type %s = %ld\n", currgn, atts[i+1], games[curgame].roms[currgn].loadadr);
				}
				else if (!strcmp(atts[i], "size"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].roms[currgn].length = val;
				}
				else if (!strcmp(atts[i], "clear"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].roms[currgn].flags |= (val & 0xff)<<8;
					games[curgame].roms[currgn].flags |= RGN_CLEAR;
				}
				else if (!strcmp(atts[i], "endian"))
				{
					if (atts[i+1][0] == 'b')
					{
						games[curgame].roms[currgn].flags |= RGN_BE;
					}
					else
					{
						games[curgame].roms[currgn].flags |= RGN_LE;
					}
				}
				else printf("UNK REGION attr [%s]\n", atts[i]);
				break;

			case XML_ROM:
				if (!strcmp(atts[i], "name"))
				{
//					printf("ROM %d: ROM type %s\n", currgn, atts[i+1]);
					games[curgame].roms[currgn].name = (char *)malloc(strlen(atts[i+1])+1);
					strcpy(games[curgame].roms[currgn].name, atts[i+1]);
				}
				else if (!strcmp(atts[i], "size"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].roms[currgn].length = val;
				}
				else if (!strcmp(atts[i], "crc"))
				{
					val = strtoul(atts[i+1], &eptr, 16);
					games[curgame].roms[currgn].crc = val;
				}
				else if (!strcmp(atts[i], "offset"))
				{
					val = strtoul(atts[i+1], &eptr, 16);
					games[curgame].roms[currgn].loadadr = val;
				}
				else if (!strcmp(atts[i], "flip"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].roms[currgn].flags |= ROM_REVERSE;
				}
				else if (!strcmp(atts[i], "skip"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					games[curgame].roms[currgn].flags |= val<<12;
				}
				else if (!strcmp(atts[i], "width"))
				{
					if (atts[i+1][0] == 'd')
					{
						games[curgame].roms[currgn].flags |= ROM_DWORD;
					}
					else if (atts[i+1][0] == 'w')
					{
						games[curgame].roms[currgn].flags |= ROM_WORD;
					}
				}
				else if (!strcmp(atts[i], "sha1"))
				{
					strcpy(games[curgame].roms[currgn].sha1, atts[i+1]);
				}
				else if (!strcmp(atts[i], "md5"))
				{
					strcpy(games[curgame].roms[currgn].md5, atts[i+1]);
				}
//				else printf("UNK ROM attr [%s]\n", atts[i]);
				break;

			default:
				break;
		}
		i+=2;
	}

	*depthPtr += 1;
}

static void endElement(void *userData, const char *name)
{
	int *depthPtr = (int *)userData;
	*depthPtr -= 1;

//	printf("close [%s]: element data = [%s]\n", name, element_data);

	switch (xml_state)
	{
		case XML_TITLE:
			games[curgame].name = (char *)malloc(strlen(element_data)+1);
			strcpy(games[curgame].name, element_data);
			break;

		case XML_YEAR:
			games[curgame].year = (char *)malloc(4+1);
			strncpy(games[curgame].year, element_data, 4);
			games[curgame].year[4] = '\0';
			break;

		case XML_MAKER:
			games[curgame].mfgstr = (char *)malloc(strlen(element_data)+1);
			strcpy(games[curgame].mfgstr, element_data);
			break;

		default:
			break;
	}

	element_data[0] = '\0';
}

static void charData(void *userData, const char *s, int len)
{
	strncat(element_data, s, len);
}

// just pass through all 8-bit values for now
static int unkEncodingHandler(void *handlerData, const XML_Char *name, XML_Encoding *info)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		info->map[i] = i;
	}
	info->convert = NULL;
	info->release = NULL;

	return XML_STATUS_OK;
}

// TODO: fix this to take the m1.xml path as an option
// easy fix, just take the path as a parameter to this method
// also need to fix m1snd to explicitly call this only when
// we need to update for a new XML

// I think I'll let m1 load everything into GameT structs like it does now
// when the XML parse is done I'll go back through and pass each GameT
// to Java. since this only happens on first run or when the user
// explicitly tell us to this should be ok performance wise.
// then when we load a game I can just pass the GameT and the id(do I even need to do this?)
// back to native and let m1 handle it from there
int gamelist_load(char *basepath)
{
	XML_Parser parser = XML_ParserCreate(NULL);
	FILE *f;
	int depth = 0, done;
	static char buf[BUFSIZ];

	curgame = -1;
	numgames = 0;
	games = (M1GameT *)NULL;	// realloc() will become malloc() if ptr is NULL, which is handy
	char *xmlpath = (char *)malloc(512);
	sprintf(xmlpath, "%s/m1.xml\0", basepath);
	f = fopen(xmlpath, "r");
	if (!f)
	{
		return 0;
	}

	XML_SetUserData(parser, &depth);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, charData);
	XML_SetUnknownEncodingHandler(parser, unkEncodingHandler, NULL);

	xml_state = XML_INVALID;
		
	do 
	{
		unsigned int len = fread(buf, 1, sizeof(buf), f);

		done = (len < sizeof(buf));

		if (!XML_Parse(parser, buf, len, done))
		{
			printf("XML parse error %s at line %d (m1.xml)\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser));

			// clean up
			XML_ParserFree(parser);
			if (games)
			{
				free(games);
				games = (M1GameT *)NULL;
			}
			return 0;
		}

	} while (!done);

	XML_ParserFree(parser);

	fclose(f);

//	printf("Finished loading, %d games\n", numgames);
	return numgames;
}


