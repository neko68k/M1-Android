/* --------------------------------
 * trklist.cpp - track list support
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "m1snd.h"
#include "trklist.h"
#include "m1ui.h"

#include "expat.h"

static TrkT *curlist = (TrkT *)NULL;
static TrkT *curnode = (TrkT *)NULL;
static int defsong, songmax, numsongnames, version, fixed_vol;
static int cur_lang = 0;
static char element_data[2048];

typedef enum
{
	XML_INVALID = 0,
	XML_GOTM1LIST,
	XML_SONG,
	XML_TITLE,
	XML_NOTE,
	XML_COMPLETE,
} XMLStateE;

static XMLStateE xml_state;

char *lang[M1_LANG_MAX] =
{
	(char *)"en", (char *)"jp", (char *)"ko", (char *)"cn", (char *)"es", (char *)"fr", (char *)"nl", (char *)"be", (char *)"de",
	(char *)"pl", (char *)"se", (char *)"no", (char *)"fi", (char *)"ru", (char *)"pt", (char *)"br",
};

#define EATWHITESPACE	\
	while ((linebuffer[i] == ' ' || linebuffer[i] == 0x09 || linebuffer[i] == '\r') && (i < 512))	\
	{	\
		i++;	\
	}

#define EATPASTEQUALS	\
	while ((linebuffer[i] != '=') && (i < 512))	\
	{	\
		i++;	\
	}	\
	i++;

#define EATPASTQUOTE	\
	while ((linebuffer[i] != '"') && (i < 512))	\
	{	\
		i++;	\
	}	\
	i++;

#define EATPASTBRACKET	\
	while ((linebuffer[i] != '<') && (i < 512))	\
	{	\
		i++;	\
	}	\
	i++;

#define EATPASTDASH	\
	while ((linebuffer[i] != '-') && (i < 512))	\
	{	\
		i++;	\
	}	\
	i++;

#define TOKENIZE	\
	while ((isalnum(linebuffer[i]) || (linebuffer[i] == '_')) && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZETOQUOTE	\
	while ((linebuffer[i] != '"') && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZENUM	\
	if (linebuffer[i] == '$')	\
	{	\
		token[j++] = '0';	\
		token[j++] = 'x';	\
		i++;			\
	}	\
	else if (linebuffer[i] == '#')	\
	{	\
		i++;	\
	}	\
	if (linebuffer[i] == '0')	\
	{	\
		while ((linebuffer[i] == '0') && (i < 512))	\
		{	\
			i++;	\
		}	\
	}\
	while (isalnum(linebuffer[i]) && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#if 0
static int trklist_hasdash(char *linebuffer)
{
	while (*linebuffer != '\0')
	{
		if (*linebuffer == '-')
		{
			return 1;
		}

		linebuffer++;
	}

	return 0;
}
#endif

// trklist_insert: insert a node into the list
static void trklist_insert(TrkT *newnode)
{
	TrkT *insnode;

	// insert us in the list in sort order
	if (curlist == (TrkT *)NULL)
	{	
		// no existing list, blind-insert
		curlist = newnode;
	}
	else
	{
		// if we're smaller than the smallest track # in the list,
		// insert us there
		insnode = curlist;
		while (insnode->next != (TrkT *)NULL)
		{
			insnode = insnode->next;
		}

		// we've got the end of the list.
		insnode->next = newnode;
	}
}

// trklist_addtrk: adds a track to the list
static void trklist_addtrk(int tnum, char *linebuffer)
{
	TrkT *newnode;
	char token[128];
	int i, j;

	i = j = 0;

	// eliminate undefined behavior (found by Valgrind)
	memset(token, 0, sizeof(token));

	// allocate and initialize the new node
	newnode = (TrkT *)malloc(sizeof(TrkT));

	newnode->number = tnum;
	newnode->name[0] = '\0';
	newnode->frames = -1;
	newnode->next = (TrkT *)NULL;
	newnode->curextra = 0;
	
	while ((linebuffer[i] != 0x0a) && (linebuffer[i] != 0x0d) && (linebuffer[i] != '<') && (i < 512) && (j < 128))
	{
		// handle simple HTML entities
		if ((linebuffer[i] == '&') && (linebuffer[i+1] == 'g') && (linebuffer[i+2] == 't') && (linebuffer[i+3] == ';'))
		{
			token[j] = '>';
			i += 3;
		}
		else if ((linebuffer[i] == '&') && (linebuffer[i+1] == 'l') && (linebuffer[i+2] == 't') && (linebuffer[i+3] == ';')) 
		{
			token[j] = '<';
			i += 3;
		}
		else if ((linebuffer[i] == '&') && (linebuffer[i+1] == 'a') && (linebuffer[i+2] == 'm') && (linebuffer[i+3] == 'p') && (linebuffer[i+4] == ';')) 
		{
			token[j] = '&';
			i += 4;
		}
		else if ((linebuffer[i]&0xe0) == 0xc0)	// UTF-8 2-byte character
		{
			token[j++] = linebuffer[i++];
			token[j] = linebuffer[i];
		}
		else if ((linebuffer[i]&0xf0) == 0xe0)	// UTF-8 3-byte character
		{
			token[j++] = linebuffer[i++];
			token[j++] = linebuffer[i++];
			token[j] = linebuffer[i];
		}
		else if ((linebuffer[i]&0xf8) == 0xf0)	// UTF-8 4-byte character
		{
			token[j++] = linebuffer[i++];
			token[j++] = linebuffer[i++];
			token[j++] = linebuffer[i++];
			token[j] = linebuffer[i];
		}
		else
		{
			token[j] = linebuffer[i];
		}

		i++;
		j++;
	}
	token[j] = '\0';

	// reject empty tokens
	if (token[0] == '\n' || token[0] == '\r' || token[0] == '\0')
	{
		free(newnode);
		return;
	}

	// eat trailing spaces
	j--;
	while ((isspace(token[j]) || iscntrl(token[j])))
	{
		j--;
	}
	j++;
	token[j] = '\0';

	strcpy(newnode->name, token);

	// check for track length
	if (linebuffer[i] == '<')
	{
		double minutes, seconds;

		// take care of the opening bracket
		EATPASTBRACKET

		if (version == 1)
		{
			j = 0;
			while ((linebuffer[i] !=':') && (linebuffer[i] != '\0') && (i < 512))
			{
				token[j++] = linebuffer[i++];
			}
			minutes = strtod(token, NULL);
			i++;	// skip the :
			j = 0;
			while ((linebuffer[i] !='>') && (linebuffer[i] != '\0') && (i < 512))
			{
				token[j++] = linebuffer[i++];
			}
			seconds = strtod(token, NULL);
		
			newnode->frames = (long)((minutes * 60.0 + seconds) * 60.0);
		}
		else if (version == 2)
		{
			// version 2 files have an XML-like string of tokens
// 			printf("v2 trk length [%s] =", &linebuffer[i]);
			while ((linebuffer[i] != '>') && (i < 512) && (linebuffer[i] != 0x0a) && (linebuffer[i] != 0x0d))
			{
				EATWHITESPACE
				j = 0;
				TOKENIZE
				j = 0;
				EATPASTEQUALS
				EATPASTQUOTE

				if (token[0] != '\0')
				{
					// handle known tokens
					if (!strcmp(token, "time"))
					{
						int orig_i = i;

						j = 0;
						while ((linebuffer[i] !=':') && (linebuffer[i] != '\0') && (i < 512))
						{
							token[j++] = linebuffer[i++];
						}

						// check if there was actually a colon
						if (linebuffer[i] == ':')
						{
							minutes = strtod(token, NULL);
							i++;	// skip the :
						}
						else
						{
							// no colon, seconds only
							minutes = 0;
							i = orig_i;	// go ahead and slurp seconds
						}

						j = 0;
						while ((linebuffer[i] !='"') && (linebuffer[i] != '\0') && (i < 512))
						{
							token[j++] = linebuffer[i++];
						}
						seconds = strtod(token, NULL);

//						printf("%f mins %f secs\n", minutes, seconds);

						newnode->frames = (long)((minutes * 60.0 + seconds) * 60.0);

						EATPASTQUOTE
					}
					else if (!strcmp(token, "fadeout"))
					{
						j = 0;
						TOKENIZETOQUOTE
						EATPASTQUOTE

						seconds = strtod(token, NULL);
						newnode->fadeout = (long)seconds;
					}
					else
					{
						j = 0;
						TOKENIZETOQUOTE
						EATPASTQUOTE
				 	}
				}
			}
		}
		else
		{
			// unknown file version, ignore
			printf("Unknown .lst file version %d\n", version);
		}
	}

	trklist_insert(newnode);

//	printf("track %04d is [%s]\n", tnum, token);

	numsongnames++;
}

static void trklist_addextra(int tnum, char *text)
{
	TrkT *node;

	node = curlist;

	if ((*text == '\0') || (*text == 0x0a) || (*text == 0x0d))
	{
		return;
	}

	while (node != (TrkT *)NULL)
	{
		if (node->number == tnum)
		{
			// got it
			if (node->curextra < 128)
			{
				char *dst;

				node->extra[node->curextra] = (char *)malloc(strlen(text)+1);
				dst = node->extra[node->curextra];
				while ((*text != 0x0a) && (*text != 0x0d))
				{	// handle HTML entities
					if (*text == '&')
					{
						if (text[1] == 'l' && text[2] == 't' && text[3] == ';')
						{
							*dst++ = '<';
							text += 4;
						}
						else if (text[1] == 'g' && text[2] == 't' && text[3] == ';')
						{
							*dst++ = '>';
							text += 4;
						}
						else if (text[1] == 'a' && text[2] == 'm' && text[3] == 'p' && text[4] == ';')
						{
							*dst++ = '&';
							text += 5;
						}
						else 
						{
//							printf("Unknown amp [%c%c]\n", text[1], text[2]);
							*dst++ = *text++;
						}
					}
					else if ((*text&0xe0) == 0xc0)	// UTF-8 2-byte character
					{
						*dst++ = *text++;
						*dst++ = *text++;
					}
					else if ((*text&0xf0) == 0xe0)	// UTF-8 3-byte character
					{
						*dst++ = *text++;
						*dst++ = *text++;
						*dst++ = *text++;
					}
					else if ((*text&0xf8) == 0xf0)	// UTF-8 4-byte character
					{
						*dst++ = *text++;
						*dst++ = *text++;
						*dst++ = *text++;
						*dst++ = *text++;
					}
					else
					{
						*dst++ = *text++;
					}
				}
//				printf("Adding extra [%s] for trk %d\n", node->extra[node->curextra], tnum);

				node->curextra++;
				return;
			}
/*			else
			{
				logerror("ERROR: tnum %d has > 128 extras\n");
			}*/

		}

		node = node->next;
	}
}

static void startElement(void *userData, const char *name, const char **atts)
{
	int i, val;
	int *depthPtr = (int *)userData;
	char *eptr;

	printf("%s\n", name);

	element_data[0] = '\0';

	if (!strcmp(name, "m1list"))		xml_state = XML_GOTM1LIST;
	else if (!strcmp(name, "song"))
	{
		// if not the first song, link the current node and make a new one
		if (xml_state != XML_GOTM1LIST)
		{
			trklist_insert(curnode);

			curnode = (TrkT *)malloc(sizeof(TrkT));
			curnode->number = 0;
			curnode->name[0] = '\0';
			curnode->frames = -1;
			curnode->next = (TrkT *)NULL;
			curnode->curextra = 0;
		}
		

		xml_state = XML_SONG;
	}
	else if (!strcmp(name, "title"))	xml_state = XML_TITLE;
	else if (!strcmp(name, "note"))		xml_state = XML_NOTE;

	i = 0;
	while (atts[i] != (const char *)NULL)
	{
//		printf("%s = [%s]\n", atts[i], atts[i+1]);
		switch (xml_state)
		{
			case XML_GOTM1LIST:
				if (!strcmp(atts[i], "version"))
				{
					val = strtoul(atts[i+1], &eptr, 0);
					if (val > M1_XML_VERSION)
					{
						printf("Unknown future XML file version!  Attempting parse anyway.\n");
					}
					else
					{
//						printf("XML version OK\n");
					}
				}
				else if (!strcmp(atts[i], "numsongs"))
				{
					curnode->numsongs = strtoul(atts[i+1], &eptr, 0);
//					printf("# songs = %d\n", curnode->numsongs);
				}
				else if (!strcmp(atts[i], "defcmd"))
				{
					curnode->defcmd = strtoul(atts[i+1], &eptr, 0);
//					printf("defcmd = %d\n", curnode->numsongs);
				}
				break;

			case XML_SONG:
				if (!strcmp(atts[i], "num"))
				{
					curnode->number = strtoul(atts[i+1], &eptr, 0); 
					//printf("song # = %d\n", curnode->number);
				}
				if (!strcmp(atts[i], "vol"))
				{
					// volume attribute...
				}
				break;

			case XML_TITLE:	// no atts defined for title
			case XML_NOTE:	// no atts defined for note
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
			strcpy(curnode->name, element_data);
			xml_state = XML_SONG;
			break;

		case XML_NOTE:
			curnode->extra[curnode->curextra] = (char *)malloc(strlen(element_data)+1);
			strcpy(curnode->extra[curnode->curextra], element_data);
			curnode->curextra++;
			xml_state = XML_SONG;
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

static TrkT *trklist_getnode(TrkT *list, int num)
{
	TrkT *node;

	node = list;

	while (node != (TrkT *)NULL)
	{
		if (node->number == num)
		{
			return node;
		}

		node = node->next;
	}

	return (TrkT *)NULL;
}

// trklist_load_xml: loads an XML format tracklist
TrkT *trklist_load_xml(FILE *f, char *base)
{
	XML_Parser parser = XML_ParserCreate(NULL);
	int done;
	int depth = 0;
	static char buf[BUFSIZ];
	TrkT *newnode;
	
	XML_SetUserData(parser, &depth);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, charData);
	XML_SetUnknownEncodingHandler(parser, unkEncodingHandler, NULL);

	// allocate and initialize the first node
	newnode = (TrkT *)malloc(sizeof(TrkT));
	newnode->number = 0;
	newnode->name[0] = '\0';
	newnode->frames = -1;
	newnode->next = (TrkT *)NULL;
	newnode->curextra = 0;

	xml_state = XML_INVALID;
	curnode = newnode;

	do 
	{
		unsigned int len = fread(buf, 1, sizeof(buf), f);

		done = (len < sizeof(buf));

		if (!XML_Parse(parser, buf, len, done))
		{
			printf("XML parse error %s at line %d (%s.xml)\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser), base);

			// clean up
			XML_ParserFree(parser);
			free(newnode);
			newnode = (TrkT *)NULL;
			return newnode;
		}

	} while (!done);

	XML_ParserFree(parser);

	fclose(f);
	return newnode;
}

// trklist_load: loads a game's tracklist.  Tries .lst first then .xml.
TrkT *trklist_load(char *base)
{
	FILE *f;
	static char path[512];
	static char linebuffer[2048];
	int i, j, end, state, cursong;
	char token[128];


// TODO: fix this to work on android also
// change to directory we set in front end options
        #if __MACOSX__
        change_to_app_directory();
        #endif

	defsong = songmax = fixed_vol = -1;
	numsongnames = 0;
	curlist = (TrkT *)NULL;
	version = 1;	// assume version 1

	// if both exist, use the xml list
	#if __WIN32__
	sprintf(path, "lists\\%s\\%s.xml", lang[cur_lang], base);
	#else
	sprintf(path, "/sdcard/m1/lists/%s/%s.xml", lang[cur_lang], base);
	#endif

	f = fopen(path, "r");
	if (f != (FILE *)NULL)
	{
		return trklist_load_xml(f, base);
	}

	// no xml list, try the classic one
	#if __WIN32__
	sprintf(path, "lists\\%s\\%s.lst", lang[cur_lang], base);
	#else
	sprintf(path, "/sdcard/m1/lists/%s/%s.lst", lang[cur_lang], base);
	#endif

	f = fopen(path, "r");
	if (f == (FILE *)NULL)
	{
		curlist = (TrkT *)NULL;
		return curlist;
	}

//	printf("Loading track names for %s\n", base);
//	fflush(stdout);

	// yay, we've got it
	end = 0;
	state = 0;
	cursong = -1;
	while (!end && !feof(f))
	{
		// flush the linebuffer and get the next line
		memset(linebuffer, 0, 512);
		fgets(linebuffer, 512, f);
		// if we're now at EOF, stop processing
		if (!feof(f))
		{
			i = 0;
			EATWHITESPACE

			switch (linebuffer[i])
			{
				case '$':
					j = 0;
					switch (state)
					{
						case 0:	// file header
							i++;	// skip the dollar sign
							TOKENIZE
							if (!strcmp(token, "name"))
							{
								// not sure what use this is really
								EATPASTEQUALS
								j = 0;
								TOKENIZE
							}

							if (!strcmp(token, "default"))
							{
								EATPASTEQUALS
								EATWHITESPACE
								j = 0;
								TOKENIZENUM
								defsong = strtoul(token, NULL, 0);
							}

							if (!strcmp(token, "fixed_volume"))
							{
								EATPASTEQUALS
								EATWHITESPACE
								j = 0;
								TOKENIZENUM
								fixed_vol = strtoul(token, NULL, 0);
							}

							if (!strcmp(token, "version"))
							{
								EATPASTEQUALS
								EATWHITESPACE
								j = 0;
								TOKENIZENUM
								version = strtoul(token, NULL, 0);
//								printf("Switching parse method for %s.lst to file version %d\n", base, version);
							}

							if (!strcmp(token, "songmax"))
							{
								EATPASTEQUALS
								EATWHITESPACE
								j = 0;
								TOKENIZENUM
								songmax = strtoul(token, NULL, 0);
							}

							if (!strcmp(token, "main"))
							{
								state = 1;
							}
							break;

						case 1:	// actual songlist
//							printf("[%c %c %c]\n", linebuffer[i], linebuffer[i+1], linebuffer[i+2]);
							if ((linebuffer[i+1] == 'e') && (linebuffer[i+2] == 'n') && (linebuffer[i+3] == 'd'))
							{
								end = 1;
							}
							else
							{
								j = 0;
								TOKENIZENUM
								cursong = strtoul(token, NULL, 0);
								EATWHITESPACE
								trklist_addtrk(cursong, &linebuffer[i]);
							}
							break;
					}
					break;

				case '#':
					j = 0;
					TOKENIZENUM
					cursong = strtoul(token, NULL, 0);
					EATWHITESPACE
					trklist_addtrk(cursong, &linebuffer[i]);
					break;

				default: 
					if (cursong != -1)
					{
						if ((linebuffer[i] == '/') && (linebuffer[i+1] == '/'))
						{
						}	// ignore comments
						else
						{
							trklist_addextra(cursong, linebuffer);
						}
					}
					break;
			}
		}
	}

#if 0
	// dump the songlist
	{
		TrkT *list = curlist;

		while (list != (TrkT *)NULL)
		{
//			printf("Song %04d: %s\n", list->number, list->name);

			list = list->next;
		}
	}
#endif

	if (curlist)
	{
		// if no default song is specified, default to the first entry
		if (defsong == -1)
		{
			defsong = curlist->number;
		}

		curlist->numsongs = numsongnames;
		curlist->defcmd = defsong;
		curlist->fixed_vol = fixed_vol;
	}

	fclose(f);

//	printf("%s trklist = %x\n", base, (unsigned int)curlist);
	return curlist;
}

// trklist_unload: unloads a game's tracklist
void trklist_unload(TrkT *ltodel)
{
	TrkT *node, *next;
	int i;

	if (ltodel == (TrkT *)NULL)
	{
		return;
	}

	node = ltodel;
	while (node != (TrkT *)NULL)
	{
		// free the extras list
		if (node->curextra)
		{
			for (i = 0; i < node->curextra; i++)
			{
				free(node->extra[i]);
			}
		}

		next = node->next;
		free((void *)node);
		node = next;
	}

	curlist = (TrkT *)NULL;
}

// trklist_getname: gets the name for a track
char *trklist_getname(TrkT *list, int num)
{
	TrkT *node;

	node = trklist_getnode(list, num);
	
	if (node != (TrkT *)NULL)
	{
		return node->name;
	}

	return (char *)NULL;
}

// trklist_getextra: gets the extra text for a track
char *trklist_getextra(TrkT *list, int num, int numextra)
{
	TrkT *node;

	node = trklist_getnode(list, num);
	
	if (node != (TrkT *)NULL)
	{
		return node->extra[numextra];
	}

	return (char *)NULL;
}

// trklist_getnumextras: get number of extra lines of text for a track
long trklist_getnumextras(TrkT *list, int num)
{
	TrkT *node;

	node = trklist_getnode(list, num);
	
	if (node != (TrkT *)NULL)
	{
		return node->curextra;
	}

	return -1;
}

// trklist_getlength: gets the length for a track in 1/60 second units
long trklist_getlength(TrkT *list, int num)
{
	TrkT *node;

	node = trklist_getnode(list, num);
	
	if (node != (TrkT *)NULL)
	{
		return node->frames;
	}

	return -1;
}

// trklist_getnumsongs: gets the number of "named" songs
int trklist_getnumsongs(TrkT *list)
{
	if (list == (TrkT *)NULL)
	{
		return 0;
	}

	return list->numsongs;
}

// trklist_getsongnum: translate a named song number to a command number
int trklist_song2cmd(TrkT *list, int num)
{
	TrkT *node;
	int rv = 0;

	node = list;

	while (node != (TrkT *)NULL)
	{
		if (rv == num)
		{
			return node->number;
		}

		rv++;
		node = node->next;
	}

	return -1;
}

// trklist_getdefcmd: gets the default command # we want
int trklist_getdefcmd(TrkT *list)
{
	if (list == (TrkT *)NULL)
	{
		return 0;
	}

	return list->defcmd;
}

// trklist_getfixedvol: gets the fixed volume for a song
int trklist_getfixvol(TrkT *list)
{
	if (list == (TrkT *)NULL)
	{
		return -1;
	}

	return list->fixed_vol;
}

// trklist_cmd2song: translate command number to a named song number
int trklist_cmd2song(TrkT *list, int num)
{
	TrkT *node;
	int rv = 0;

	node = list;

	while (node != (TrkT *)NULL)
	{
		if (node->number == num)
		{
			return rv;
		}

		rv++;
		node = node->next;
	}

	return 0;
}

// trklist_setlang: sets the language code
void trklist_setlang(int lang)
{
	if (lang >= 0 && lang < M1_LANG_MAX)
	{
		cur_lang = lang;
	}
}

// trklist_getlang: returns the current language
int trklist_getlang(void)
{
	return cur_lang;
}

// trklist_setname: sets the name of a selected track
TrkT *trklist_setname(TrkT *list, int songnum, char *name)
{
	TrkT *newlist, *songnode;

	if (trklist_getname(list, songnum) != (char *)NULL)
	{
		// easy case, it already exists, just change it
		curlist = newlist = list;
		songnode = trklist_getnode(list, songnum);
		strcpy(songnode->name, name);
	}
	else
	{
		curlist = list;
		trklist_addtrk(songnum, name);
		newlist = curlist;
	}

	return newlist;
}

TrkT *trklist_setextra(TrkT *list, int songnum, int extranum, char *extratxt)
{
	TrkT *newlist;
	char tmp[256], *eptr;

	if (trklist_getname(list, songnum) != (char *)NULL)
	{
		// easier case, the song node at least already exists...
		curlist = newlist = list;

		// see if there's an extra in that slot yet
		eptr = trklist_getextra(list, songnum, extranum);

		// if no extra exists in that slot, just add it 
		// (we cannot allow the extra list to fragment!)
		if (eptr == NULL)
		{
			trklist_addextra(songnum, extratxt);
		}
		else
		{
			// one exists, let's do it...
			strcpy(eptr, extratxt);
		}
	}
	else
	{
		curlist = list;
		sprintf(tmp, "Song %d", songnum);
		trklist_addtrk(songnum, tmp);
		trklist_addextra(songnum, extratxt);
		newlist = curlist;
	}

	return newlist;
}










 
