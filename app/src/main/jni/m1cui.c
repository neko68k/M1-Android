/*

m1cui.c - M1 command line user interface

Copyright (c) 2001-2007 R. Belmont.

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if __linux__
#include <termios.h>
#include <unistd.h>
#define HAVE_ICONV	(0)	// can use iconv and output UTF-8

extern int lnxdrv_apimode;
#endif

#if MACOS
#include <termios.h>
#include <unistd.h>
#define HAVE_ICONV	(0)
#endif

#if WIN32
#include <windows.h>
#include <conio.h>
#define HAVE_ICONV	(0)
#endif

#include "m1ui.h"

#if HAVE_ICONV
#include <iconv.h>
#endif

// version check
#if M1UI_VERSION != 20
#error M1UI VERSION MISMATCH!
#endif

static int max_games;
static int quit;
static char *rompath, *wavpath;

static int m1ui_message(void *this, int message, char *txt, int iparm);

// Private functions

// convert a string from one encoding to another
#if HAVE_ICONV
void char_conv(char *src, char *dst, int dlen)
{
	char *pin, *pout;
	size_t inbytes, outbytes;
	iconv_t iconvHandle;

	pin = src;
	pout = dst;
	inbytes = strlen(src);
	outbytes = dlen;

	iconvHandle = iconv_open("UTF-8", m1snd_get_info_str(M1_SINF_ENCODING, 0));
	if (iconvHandle == (iconv_t)-1)
	{
		printf("Invalid encoding!\n");
	}
	iconv(iconvHandle, &pin, &inbytes, &pout, &outbytes);
	iconv_close(iconvHandle);
}
#endif

// check if a line contains a token of the form "[xxx]"
static int istoken(char *line, char *token)
{
	char *p;

	// eat whitespace/control characters/etc
	p = line;
	while ((iscntrl(*p) || isspace(*p)) && (*p != '\0'))
	{
		p++;
	}

	while (*p != '\0')
	{
		// ignore comment lines
		if (*p == ';')
		{
			return 0;
		}

		// if we find a [ it's a token line, maybe
		if (*p == '[')
		{
			// got it!  skip over it
			p++;
			if (!strncasecmp(p, token, strlen(token)))
			{
				return 1;
			}
		}

		p++;
	}

	return 0;	
}

// strips down a buffer to only text
static void strip_down(char *line)
{
	char *p;
	char *dest;

	// eat beginning whitespace
	p = line;
	dest = line;
	while ((iscntrl(*p) || isspace(*p)) && (*p != '\0'))
	{
		p++;
	}

	// copy over all alphanumerics and spaces
	while (!iscntrl(*p) && *p != '\0')
	{
		*dest++ = *p++;
	}

	*dest++ = '\0';
}

// read and process the config (.ini) file
static void read_config(FILE *cnf)
{
	char linebuf[8192], *p;
	int state;

	// init to "no token" state
	state = 0;

	// scan the entire .ini file
	while (!feof(cnf))
	{
		fgets(linebuf, 8192, cnf);

		if (!feof(cnf))
		{
			// check for a blank line by eating any initial whitespace
			p = linebuf;
			while ((iscntrl(*p) || isspace(*p)) && (*p != '\0'))
			{
				p++;
			}

			// if we're now at EOL, it's blank so reset state to 0
			// also handle comment lines that way
			if ((*p == '\0') || (*p == ';'))
			{
				state = 0;
			}
			else if (istoken(linebuf, "ROMPATH"))
			{
				state = 1;
			}
			else if (istoken(linebuf, "WAVPATH"))
			{
				state = 2;
			}
			else
			{
				switch (state)
				{
					case 0:	      	// do nothing in state 0
						break;

					case 1:	// in "rompath" state, add line to rompath
						strip_down(linebuf);
						strcat(rompath, linebuf);
						strcat(rompath, ";");
						break;

					case 2:	// in "wavpath" state, add line to wavpath
						strip_down(linebuf);
						strcpy(wavpath, linebuf);
						#if __WIN32__
						strcat(wavpath, "\\");
						#else
						strcat(wavpath, "/");
						#endif

						// wavpath can only have one element, fall out of state 2
						state = 0;
						break;
				}
			}
		}
	}
}

// find_rompath: returns if the file can be found in the ROM search path
// alters "fn" to be the full path if found so make sure it's big
static int find_rompath(char *fn)
{
	static char curpath[16384];
	char *p;
	FILE *f;
	int i;

	p = rompath;
	while (*p != '\0')
	{
		// copy the path
		i = 0;
		while (*p != ';' && *p != '\0')
		{
			curpath[i++] = *p++;
		}
		curpath[i] = '\0';

		// p now points to the semicolon, skip that for next iteration
		p++;

		strcat(curpath, "/");	// path separator
		strcat(curpath, fn);

		f = fopen(curpath, "rb");
		if (f != (FILE *)NULL)
		{
			// got it!
			fclose(f);
			m1snd_set_info_str(M1_SINF_SET_ROMPATH, curpath, 0, 0, 0);
			return 1;
		}
	}	

	return 0;
}

// Public functions

// callbacks from the core of interest to the user interface
static int m1ui_message(void *this, int message, char *txt, int iparm)
{
	int curgame;

	switch (message)
	{
		// called when switching to a new game
		case M1_MSG_SWITCHING:
			printf("\nSwitching to game %s\n", txt);
			break;

		// called to show the current game's name
		case M1_MSG_GAMENAME:
			curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			printf("Game selected: %s (%s, %s)\n", txt, m1snd_get_info_str(M1_SINF_MAKER, curgame), m1snd_get_info_str(M1_SINF_YEAR, curgame));
			break;

		// called to show the driver's name
		case M1_MSG_DRIVERNAME:
			printf("Driver: %s\n", txt);
			break;

		// called to show the hardware description
		case M1_MSG_HARDWAREDESC:
			printf("Hardware: %s\n", txt);
			break;

		// called when ROM loading fails for a game
		case M1_MSG_ROMLOADERR:
			printf("ROM load error, bailing\n");
			exit(-1);
			break;

		// called when a song is (re)triggered
		case M1_MSG_STARTINGSONG:
			curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			if (m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | curgame) == (char *)NULL)
			{
				printf("Starting song #%d\n", iparm);
			}
			else
			{
				int i;
				char *rawname, transname[512];

				#if HAVE_ICONV
				rawname = m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | curgame);
//				printf("rawname = [%s]\n", rawname);
				memset(transname, 0, 512);
				char_conv(rawname, transname, 512);
				#else	// pass-through
				rawname = m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | curgame);
				strcpy(transname, rawname);
				#endif

				printf("Starting song #%d: %s\n", iparm, transname);

				if (m1snd_get_info_int(M1_IINF_NUMEXTRAS, (iparm<<16) | curgame) > 0)
				{
					for (i = 0; i < m1snd_get_info_int(M1_IINF_NUMEXTRAS, (iparm<<16) | curgame); i++)
					{
						#if HAVE_ICONV
						rawname = m1snd_get_info_str_ex(M1_SINF_EX_EXTRA, curgame, iparm, i);
						memset(transname, 0, 512);
						char_conv(rawname, transname, 512);
						printf("%s\n", transname);
						#else
						printf("%s\n", m1snd_get_info_str_ex(M1_SINF_EX_EXTRA, curgame, iparm, i));
						#endif
					}
					printf("\n");
				}
			}
			break;

		// called if a hardware error occurs
		case M1_MSG_HARDWAREERROR:
			m1snd_shutdown();

			free(rompath);
			free(wavpath);

			exit(-1);
			break;

		// called when the hardware begins booting
		case M1_MSG_BOOTING:
			printf("\nBooting hardware, please wait...");
			break;

		// called when the hardware is finished booting and is ready to play
		case M1_MSG_BOOTFINISHED:
			printf("ready!\n\n");
			printf("Press ESC to exit, + for next song, - for previous, 0 to restart current,\n");
			printf("                   * for next game, / for previous, space to pause/unpause\n\n");

			#if 0
			if (1)
			{
				int numst, st, ch;

				numst = m1snd_get_info_int(M1_IINF_NUMSTREAMS, 0);
				printf("%d streams\n", numst);

				for (st = 0; st < numst; st++)
				{
					for (ch = 0; ch < m1snd_get_info_int(M1_IINF_NUMCHANS, st); ch++)
					{
						printf("st %02d ch %02d: [%s] [vol %d] [pan %d]\n", st, ch, 
							m1snd_get_info_str(M1_SINF_CHANNAME, st<<16|ch),
							m1snd_get_info_int(M1_IINF_CHANLEVEL, st<<16|ch),
							m1snd_get_info_int(M1_IINF_CHANPAN, st<<16|ch));
					}
				}
			}
			#endif
			break;

		// called when there's been at least 2 seconds of silence
		case M1_MSG_SILENCE:
			break;

		// called to let the UI do vu meters/spectrum analyzers/etc
		// txt = pointer to the frame's interleaved 16-bit stereo wave data
		// iparm = number of samples (bytes / 4)
		case M1_MSG_WAVEDATA:
			break;

		case M1_MSG_MATCHPATH:
			return find_rompath(txt);
			break;

		case M1_MSG_GETWAVPATH:
			{
				int song = m1snd_get_info_int(M1_IINF_CURCMD, 0);
				int game = m1snd_get_info_int(M1_IINF_CURGAME, 0); 

				sprintf(txt, "%s%s%04d.wav", wavpath, m1snd_get_info_str(M1_SINF_ROMNAME, game), song);
			}
			break;

		case M1_MSG_ERROR:
			printf("%s\n", txt);
			break;
	}

	return 0;
}

static void _usage(char *appname)
{
	printf("\nM1 usage:\n");
	printf("%s [-a] [-b] [-i] [-l] [-n] [-r] [-t] [-w] gamename\n", appname);
	printf("-a: disable album mode (use of .lst files)\n");
	printf("-b: list all supported games by board\n");
	printf("-d: list all ROM info for each game, compatible with MAME -listinfo\n");
	printf("-i: show ROM info for the game\n");
	printf("-l: list all supported games\n");
	printf("-mN: set headphone mix (opposite channel blend).  0 = full stereo, 100 = full mono\n");
	printf("-n: don't normalize the sound\n");
	printf("-o: observe volume calculated by normalization\n");
	printf("-rN: set sample rate from 8000-48000 (default=44100)\n");
#if __linux__
	printf("-sN: set soundcard type (0=SDL, 1=ALSA, 2=OSS, 3=PulseAudio)\n");
#endif
	printf("-t: don't enforce the song lengths from the .lst files\n");
	printf("-vN: set fixed volume (0 = silence, 100 = normal, 300 = 3x normal)\n");
	printf("-w: enable log to .WAV file\n");
}

// Quick MAME -listinfo compatible thing -pjp
// No info about parent roms if it isn't supported, therefore ignore
// Removes duplicate roms in same set
static void listinfo(void)
{
	int i,j,k;

	// For each game
	for (i = 0; i < max_games; i++)
	{
		int parent = -1;

		printf("game (\n");
		printf("\tname %s\n", m1snd_get_info_str(M1_SINF_ROMNAME, i));
		printf("\tdescription \"%s\"\n", m1snd_get_info_str(M1_SINF_VISNAME, i));
		printf("\tmanufacturer \"%s\"\n", m1snd_get_info_str(M1_SINF_MAKER, i));

		// Only sets parent if it actually exists, otherwise ignores
		if (m1snd_get_info_str(M1_SINF_PARENTNAME, i)[0] != '\0')
			for (j = 0; j < max_games; j++)
				if (!strcmp( m1snd_get_info_str(M1_SINF_ROMNAME, j), m1snd_get_info_str(M1_SINF_PARENTNAME, i)))
					parent = j;

		if (parent != -1)
		{
			printf("\tcloneof %s\n", m1snd_get_info_str(M1_SINF_PARENTNAME, i));
			printf("\tromof %s\n", m1snd_get_info_str(M1_SINF_PARENTNAME, i));
		}

		// For each rom in the set
		for (j = 0; j < m1snd_get_info_int(M1_IINF_ROMNUM, i); j++)
		{
			int dupe = 0;
			unsigned int crc = m1snd_get_info_int(M1_IINF_ROMCRC, i|(j<<16));

			// If the name matches any earlier roms in same set then we will skip (Note that dupe crc's are shown)
			for (k = 0; k < j; k++)
				if (!strcmp(m1snd_get_info_str(M1_SINF_ROMFNAME, i|(j<<16)), m1snd_get_info_str(M1_SINF_ROMFNAME, i|(k<<16)) ))
					dupe = 1;

			if(!dupe)
			{
				int rom_in_parent=0;

				// If the crc matches any rom in parent set flag it
				if (parent != -1)
					for (k = 0; k < m1snd_get_info_int(M1_IINF_ROMNUM, parent); k++)
						if ((unsigned int)m1snd_get_info_int(M1_IINF_ROMCRC, parent|(k<<16)) == crc )
							rom_in_parent = 1;

				// Set to merge if crc matches any rom in parent
				if (rom_in_parent)
					printf("\trom ( name %s merge %s size %d crc %08x )\n",
						m1snd_get_info_str(M1_SINF_ROMFNAME, i|(j<<16)),
						m1snd_get_info_str(M1_SINF_ROMFNAME, i|(j<<16)),
						m1snd_get_info_int(M1_IINF_ROMSIZE, i|(j<<16)),
						crc);
				else
					printf("\trom ( name %s size %d crc %08x )\n",
						m1snd_get_info_str(M1_SINF_ROMFNAME, i|(j<<16)),
						m1snd_get_info_int(M1_IINF_ROMSIZE, i|(j<<16)),
						crc);
			}
		}
		printf(")\n\n"); // game ()
	}
}

int main(int argc, char *argv[])
{
	long i, j, info;
	char *eptr;
	char ch = 0;
	int argnum = 1;
	#if __linux__ || MACOS
	struct termios tp;
	struct timeval tv;
	int fds;
	fd_set watchset;
	#endif
	int current;
	FILE *cnf;
	int pause = 0;
	int usetime = 1;
	int sr, shownorm, lastnorm;

	m1snd_setoption(M1_OPT_RETRIGGER, 0);
	m1snd_setoption(M1_OPT_WAVELOG, 0);
	m1snd_setoption(M1_OPT_NORMALIZE, 1);
	m1snd_setoption(M1_OPT_LANGUAGE, M1_LANG_EN);
	m1snd_setoption(M1_OPT_RESETNORMALIZE, 0);

	quit = 0;
	info = 0;
	shownorm = 0;
	lastnorm = -1;

	m1snd_init(NULL, m1ui_message);
	max_games = m1snd_get_info_int(M1_IINF_TOTALGAMES, 0);

	printf("\nM1: arcade video and pinball sound emu by R. Belmont\n");
	printf("Core ver %s, CUI ver 2.4\n", m1snd_get_info_str(M1_SINF_COREVERSION, 0)); 
	printf("Copyright (c) 2001-2010.  All Rights Reserved.\n\n");

	cnf = fopen("m1.ini", "rt");
	if (!cnf)
	{
		printf("No config file found, using defaults\n");
		rompath = (char *)malloc(512);
		strcpy(rompath, "roms;");	// default rompath
		wavpath = (char *)malloc(512);
		strcpy(wavpath, "wave;");	// default wavepath
	}
	else
	{	
		printf("Reading configuration from m1.ini\n");
		rompath = (char *)malloc(65536);
		rompath[0] = '\0';
		wavpath = (char *)malloc(65536);
		wavpath[0] = '\0';
		read_config(cnf);
		fclose(cnf);
	}

	// first check and handle switches
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')	// it's a switch
		{
			switch(argv[i][1])
			{
				case 'a':
				case 'A':
					m1snd_setoption(M1_OPT_USELIST, 0);
					break;

				case 'd':
				case 'D':
					listinfo();
					m1snd_shutdown();

					free(rompath);
					free(wavpath);
					return (0);
					break;

				case 'n':
				case 'N':
					m1snd_setoption(M1_OPT_NORMALIZE, 0);
					break;

				case 'r':
				case 'R':
					sr = strtoul(&argv[i][2], &eptr, 0);
					if ((sr < 8000) || (sr > 48000))
					{
						_usage(argv[0]);
						return(0);
					}
					m1snd_shutdown();
					m1snd_setoption(M1_OPT_SAMPLERATE, sr);
					m1snd_init(NULL, m1ui_message);
					break;

#if __linux__
				case 's':
				case 'S':
					lnxdrv_apimode = strtoul(&argv[i][2], &eptr, 0);
					if ((lnxdrv_apimode < 0) || (lnxdrv_apimode > 3))
					{
						_usage(argv[0]);
						return(0);
					}
				
					m1snd_shutdown();
					m1snd_init(NULL, m1ui_message);
					break;
#endif

				case 'm':
				case 'M':
					sr = strtoul(&argv[i][2], &eptr, 0);
					if ((sr < 0) || (sr > 100))
					{
						_usage(argv[0]);
						return(0);
					}
					m1snd_setoption(M1_OPT_STEREOMIX, sr);
					break;

				case 'o':
				case 'O':
					shownorm = 1;
					break;

				case 'w':
				case 'W':
					m1snd_setoption(M1_OPT_WAVELOG, 1);
					break;

				case 't':
				case 'T':
					usetime = 0;
					break;

				case 'v':
				case 'V':
					m1snd_setoption(M1_OPT_NORMALIZE, 0);
					sr = strtoul(&argv[i][2], &eptr, 0);
					if ((sr < 0) || (sr > 9000))
					{
						_usage(argv[0]);
						return(0);
					}
					m1snd_setoption(M1_OPT_FIXEDVOLUME, sr);
					break;

				case 'b':
				case 'B':
					printf("\nSupported games by hardware:\n");
					for (j = 0; j < m1snd_get_info_int(M1_IINF_MAXDRVS, 0); j++)
					{
						if (m1snd_get_info_str(M1_SINF_BNAME, j))
						{
							printf("\nBoard: %s\n", m1snd_get_info_str(M1_SINF_BNAME, j));
						}
						else
						{
							printf("\nBoard type %ld\n", j+1);
						}
						for (i = 0; i < max_games; i++)
						{
							if (m1snd_get_info_int(M1_IINF_BRDDRV, i) == j)
							{
								if (m1snd_get_info_int(M1_IINF_HASPARENT, i))
								{
									printf("%s: %s (parent %s.zip)\n",
										m1snd_get_info_str(M1_SINF_ROMNAME, i),
										m1snd_get_info_str(M1_SINF_VISNAME, i),
										m1snd_get_info_str(M1_SINF_PARENTNAME, i));
								}
								else
								{
									printf("%s: %s\n",
										m1snd_get_info_str(M1_SINF_ROMNAME, i),
										m1snd_get_info_str(M1_SINF_VISNAME, i));
								}
							}
						}
					}

					printf("\n-------------------------\n");
					printf("%d total games supported\n", max_games);
					printf("%d total boards\n", m1snd_get_info_int(M1_IINF_MAXDRVS, 0));

					return(0);
					break;

				case 'l':
				case 'L':
					printf("\nSupported games:\n");
					for (i = 0; i < max_games; i++)
					{
						if (m1snd_get_info_int(M1_IINF_HASPARENT, i))
						{
							printf("%s: %s (parent %s.zip)\n",
								m1snd_get_info_str(M1_SINF_ROMNAME, i),
								m1snd_get_info_str(M1_SINF_VISNAME, i),
								m1snd_get_info_str(M1_SINF_PARENTNAME, i));
						}
						else
						{
							printf("%s: %s\n",
								m1snd_get_info_str(M1_SINF_ROMNAME, i),
								m1snd_get_info_str(M1_SINF_VISNAME, i));
						}
					}

					printf("\n-------------------------\n");
					printf("%d total games supported\n", max_games);
					printf("%d total hardware platforms\n", m1snd_get_info_int(M1_IINF_MAXDRVS, 0));

					return(0);
					break;

				case 'h':
				case 'H':
				case '?':
					_usage(argv[0]);
					return(0);
					break;

				case 'i':
				case 'I':
					info = 1;
					break;

				default:
					fprintf(stderr, "Unknown option %c\n", argv[i][1]);
					break;
			}
		}
		else
		{
			argnum = i;
			break;
		}
	}

	if (argc > argnum)
	{
		int gotone = 0;

		for (i = 0; i < max_games; i++)
		{
			if (!strcasecmp(argv[argnum], m1snd_get_info_str(M1_SINF_ROMNAME, i)))
			{
				if (info)
				{
					printf("\nThis is a list of the ROMs required for the game \"%s\"\n", m1snd_get_info_str(M1_SINF_ROMNAME, i));
					printf("Name              Size       Checksum\n");

					for (j = 0; j < m1snd_get_info_int(M1_IINF_ROMNUM, i); j++)
					{
						printf("%-12s  %7d bytes  %08x\n",
							m1snd_get_info_str(M1_SINF_ROMFNAME, i|(j<<16)),
							m1snd_get_info_int(M1_IINF_ROMSIZE, i|(j<<16)),
							m1snd_get_info_int(M1_IINF_ROMCRC, i|(j<<16)));
					}

					return (0);
				}

				argnum++;

				if (argc > argnum)
				{
					m1snd_setoption(M1_OPT_DEFCMD, strtoul(argv[argnum], &eptr, 0));
				}

				m1snd_run(M1_CMD_GAMEJMP, i);
				gotone = 1;
				break;
			}
		}

		if (!gotone)
		{
			fprintf(stderr, "Unknown/unsupported game %s\n", argv[argnum]);
			return(-1);
		}

		argnum++;
	}
	else
	{
		_usage(argv[0]);
		m1snd_shutdown();
		free(rompath);
		free(wavpath);
		return(0);
	}

	#if __linux__ || MACOS	// use *IX termios stuff
	tcgetattr(STDIN_FILENO, &tp);
	tp.c_lflag &= ~ICANON;
	tp.c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
	tcsetattr(STDIN_FILENO, TCSANOW, &tp);
	#endif

	// main loop
	while (!quit)
	{
		#if WIN32
		if (kbhit())
		{
			ch = getch();
		}
		else
		{
			ch = 0;
		}
		#endif
		#if __linux__ || MACOS
		fds = STDIN_FILENO;
		FD_ZERO(&watchset);
		FD_SET(fds, &watchset);
		tv.tv_sec = 0;
		tv.tv_usec = 16666/8;	// timeout every 1/480th of a second
		if (select(fds+1, &watchset, NULL, NULL, &tv))
		{
			ch = getchar();	// (blocks until something is pressed)
		}
		else
		{
			ch = 0;
		}
		#endif

		if ((usetime) && (!ch))
		{
			int curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0); 

			current = m1snd_get_info_int(M1_IINF_CURCMD, 0);

			if (m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame) != -1)
			{
//				printf("time %d / %d\n", m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame), m1snd_get_info_int(M1_IINF_CURTIME, 0));
				if (m1snd_get_info_int(M1_IINF_CURTIME, 0) >= m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame))
				{
					ch = '+';	// cheat!
//					printf("next! %d %d\n", m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame), m1snd_get_info_int(M1_IINF_CURTIME, 0));
				}
			}
		}

		switch (ch)
		{
			case 0:	// do idle-time processing
				m1snd_run(M1_CMD_IDLE, 0);

				// if -o (observe calculated normalization volumes) is on...
				if (shownorm)
				{
					if (lastnorm != m1snd_get_info_int(M1_IINF_NORMVOL, 0))
					{
						lastnorm = m1snd_get_info_int(M1_IINF_NORMVOL, 0);
						printf("vol = %d\n", lastnorm);
					}
				}
				break;

			case 'q':
			case 'Q':
			case 27:	// ESC
				quit = 1;
				break;

			case ' ':
				pause ^= 1;
				if (pause)
				{
					m1snd_run(M1_CMD_PAUSE, 0);
				}
				else
				{
					m1snd_run(M1_CMD_UNPAUSE, 0);
				}
				break;

			case '/':
				current = m1snd_get_info_int(M1_IINF_CURGAME, 0);
				if (current > 0)
				{
					m1snd_run(M1_CMD_GAMEJMP, current-1);
					lastnorm = -1;
				}
				break;

			case '*':
				current = m1snd_get_info_int(M1_IINF_CURGAME, 0);
				if (current < max_games)
				{
					m1snd_run(M1_CMD_GAMEJMP, current+1);
					lastnorm = -1;
				}
				break;

			case '+':
				current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
				if (current < m1snd_get_info_int(M1_IINF_MAXSONG, 0))
				{
					m1snd_run(M1_CMD_SONGJMP, current+1);
				}
				break;

			case '0':
				current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
				m1snd_run(M1_CMD_SONGJMP, current);
				break;

			case '-':
				current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
				if (current > m1snd_get_info_int(M1_IINF_MINSONG, 0))
				{
					m1snd_run(M1_CMD_SONGJMP, current-1);
				}
				break;

			default:
				break;
		}
	}

	m1snd_shutdown();

	free(rompath);
	free(wavpath);

	printf("\n");

	#if __linux__ || MACOS
	tcgetattr(STDIN_FILENO, &tp);
	tp.c_lflag |= ICANON;
	tp.c_lflag |= (ECHO | ECHOCTL | ECHONL);
	tcsetattr(STDIN_FILENO, TCSANOW, &tp);
	#endif

	return(0);
}

