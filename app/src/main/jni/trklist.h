#ifndef _TRKLIST_H_
#define _TRKLIST_H_

#define M1_XML_VERSION	(1)

typedef struct TrkT
{
	int number;
	char name[256];
	char *extra[128];
	long frames;
	long fadeout;

	int numsongs;
	int defcmd;
	int curextra;
	int fixed_vol;

	struct TrkT *next;
} TrkT;

TrkT *trklist_load(char *base, char *basepath);
void trklist_unload(TrkT *ltodel);
char *trklist_getname(TrkT *list, int num);
long trklist_getlength(TrkT *list, int num);
int trklist_getnumsongs(TrkT *list);
int trklist_song2cmd(TrkT *list, int num);
int trklist_cmd2song(TrkT *list, int num);
int trklist_getdefcmd(TrkT *list);
int trklist_getlang(void);
void trklist_setlang(int lang);
long trklist_getnumextras(TrkT *list, int num);
char *trklist_getextra(TrkT *list, int num, int numextra);
int trklist_getfixvol(TrkT *list);

TrkT *trklist_setname(TrkT *list, int songnum, char *name);
TrkT *trklist_setextra(TrkT *list, int songnum, int extranum, char *extratxt);
#endif
