/* xmlout.c - output an xml list file */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "m1snd.h"
#include "trklist.h"
#include "m1ui.h"

// language name table
extern char *lang[M1_LANG_MAX];

static char *encnames[M1_LANG_MAX] =
{
	(char *)"UTF-8", (char *)"Shift-JIS", (char *)"ISO-2022-KR", (char *)"Big5", (char *)"ISO-8859-1", (char *)"ISO-8859-1",
	(char *)"ISO-8859-1", (char *)"ISO-8859-1", (char *)"ISO-8859-2", (char *)"ISO-8859-2", (char *)"ISO-8859-1", 
	(char *)"ISO-8859-1", (char *)"ISO-8859-1", (char *)"Windows-1251", (char *)"ISO-8859-1", (char *)"ISO-8859-1",
};

// fix up any XML entities that appear
static void xmlout_fixstring(char *in, char *out)
{
	while (*in != '\0')
	{
//		printf("[%c] %x\n", (unsigned char)*in, (unsigned char)*in);
		switch (*in)
		{
			case '<':
				*out++ = '&';
				*out++ = 'l';
				*out++ = 't';
				*out++ = ';';
				in++;
				break;

			case '>':
				*out++ = '&';
				*out++ = 'g';
				*out++ = 't';
				*out++ = ';';
				in++;
				break;

			case '&':
				*out++ = '&';
				*out++ = 'a';
				*out++ = 'm';
				*out++ = 'p';
				*out++ = ';';
				in++;
				break;

			default:
				*out = *in;
				out++;
				in++;
				break;
		}
	}

	*out = '\0';
}

void xmlout_writelist(TrkT *list, char *base)
{
	FILE *f;
	static char path[512];
	int i;
	TrkT *node;
	static char conv[2048];

	sprintf(path, "lists/%s/%s.xml", lang[trklist_getlang()], base);

	f = fopen(path, "w");

	// print XML header
	fprintf(f, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", encnames[trklist_getlang()]);
	fprintf(f, "<m1list version=\"%d\" numsongs=\"%d\" defcmd=\"%d\">\n", M1_XML_VERSION, list->numsongs, list->defcmd);
	node = list;

	while (node != (TrkT *)NULL)
	{
		if (node->frames != -1)
		{
			int min, sec;

			// convert frames to minutes/seconds
			sec = node->frames / 60;
			min = sec / 60;
			sec %= 60;

			fprintf(f, "<song num=\"%d\" length=\"%d:%02d\">\n", node->number, min, sec);
		}
		else
		{
			fprintf(f, "<song num=\"%d\">\n", node->number);
		}
		xmlout_fixstring(node->name, conv);
		fprintf(f, "<title>%s</title>\n", conv);
		if (node->curextra > 0)
		{
			for (i = 0; i < node->curextra; i++)
			{
				xmlout_fixstring(node->extra[i], conv);
				fprintf(f, "<note>%s</note>\n", conv);
			}
		}
		fprintf(f, "</song>\n");

		node = node->next;
	}

	fprintf(f, "</m1list>\n");
	fclose(f);
}
