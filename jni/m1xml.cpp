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
	"en", "jp", "ko", "cn", "es", "fr", "nl", "be", "de",
	"pl", "se", "no", "fi", "ru", "pt", "br",
};

