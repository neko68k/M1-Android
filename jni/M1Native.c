#include <assert.h>
#include <jni.h>
#include <android/log.h>
#include <string.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "m1sdr_android.h"
#include "m1ui.h"

// version check
#if M1UI_VERSION != 20
#error M1UI VERSION MISMATCH!
#endif

static int max_games;
static char *rompath, *wavpath;
int booted = 0;
void *m1ui_this;

static int m1ui_message(void *this, int message, char *txt, int iparm);

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

jclass NDKBridge;
jmethodID callbackSilence;
jmethodID callbackSetGameName;
jmethodID callbackSetDriverName;
jmethodID callbackSetHardwareDesc;
jmethodID callbackLoadError;
jmethodID callbackStartingSong;
jmethodID callbackGenericError;
jmethodID callbackGetWaveData;
jmethodID callbackROM;
JNIEnv *cbEnv;
JavaVM *VM;

jint JNI_OnLoad(JavaVM* vm, void* reserved){
	JNIEnv *env;
	    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
	        return -1;
	    VM = vm;
	    /* get class with (*env)->FindClass */
	    /* register methods with (*env)->RegisterNatives */


	cbEnv = env;
	NDKBridge = (*env)->FindClass(env, "com/neko68k/M1/NDKBridge");

	callbackSetGameName = (*env)->GetStaticMethodID(env, NDKBridge, "SetGameName", "(Ljava/lang/String;)V");
	//callbackSetDriverName = (*env)->GetStaticMethodID(env, NDKBridge, "SetDriverName", "(Ljava/lang/String;)V");
	//callbackSetHardwareDesc = (*env)->GetStaticMethodID(env, NDKBridge, "SetHardwareDesc", "(Ljava/lang/String;)V");
	callbackLoadError = (*env)->GetStaticMethodID(env, NDKBridge, "RomLoadErr", "()V");
	//callbackStartingSong = (*env)->GetStaticMethodID(env, NDKBridge, "StartingSong", "(I)V");
	callbackGenericError = (*env)->GetStaticMethodID(env, NDKBridge, "GenericError", "(Ljava/lang/String;)V");
	callbackSilence = (*env)->GetStaticMethodID(env, NDKBridge, "Silence", "()V");
	//callbackGetWaveData = (*env)->GetStaticMethodID(env, NDKBridge, "GetWaveData", "()V");
	callbackROM = (*env)->GetStaticMethodID(env, NDKBridge, "addROM", "(Ljava/lang/String;Ljava/lang/Integer;)V");
    return JNI_VERSION_1_4;
}

// Public functions

// callbacks from the core of interest to the user interface
// this will make calls to Java statics, this !!must!! be set up
// in the init code!!!
static int m1ui_message(void *this, int message, char *txt, int iparm)
{
	JNIEnv *env;
	int curgame;
	(*VM)->GetEnv(VM, (void**) &env, JNI_VERSION_1_4);
	switch (message)
	{
		// called when switching to a new game
		case M1_MSG_SWITCHING:
			
			break;

		// called to show the current game's name
		case M1_MSG_GAMENAME:
			curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			jstring retstring = (*env)->NewStringUTF(env, m1snd_get_info_str(M1_SINF_VISNAME, curgame));
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Game: %i %s", curgame, m1snd_get_info_str(M1_SINF_VISNAME, curgame));

			(*env)->CallStaticVoidMethod(env, NDKBridge, callbackSetGameName, retstring);
			break;

		// called to show the driver's name
		case M1_MSG_DRIVERNAME:
			
			break;

		// called to show the hardware description
		case M1_MSG_HARDWAREDESC:
			
			break;

		// called when ROM loading fails for a game
		case M1_MSG_ROMLOADERR:
			//__android_log_print(ANDROID_LOG_INFO, "M1Android", "ROM Load Error!");
			//__android_log_write(ANDROID_LOG_INFO, "M1Android", txt);
			(*env)->CallStaticVoidMethod(env, NDKBridge, callbackLoadError, NULL);
			// call static to set load flag error
			// so we can catch it before we try to start playing
			break;

		// called when a song is (re)triggered
		case M1_MSG_STARTINGSONG:
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Starting song...");
			//(*env)->CallStaticVoidMethod(env, NDKBridge, callbackStartingSong, m1snd_get_info_int(M1_IINF_CURSONG, 0));
			break;

		// called if a hardware error occurs
		case M1_MSG_HARDWAREERROR:
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Hardware error, this shouldn't happen.");
			break;

		// called when the hardware begins booting
		case M1_MSG_BOOTING:
			booted = 0;
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Booting...");
			break;

		// called when the hardware is finished booting and is ready to play
		case M1_MSG_BOOTFINISHED:
			booted = 1;
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Boot Finished");
			break;

		// called when there's been at least 2 seconds of silence
		case M1_MSG_SILENCE:
			(*env)->CallStaticVoidMethod(env, NDKBridge, callbackSilence);
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
			//jstring returnString;
			//returnString = (*env)->NewStringUTF(env, txt);
			//__android_log_write(ANDROID_LOG_INFO, "M1Android", txt);
			(*env)->CallStaticVoidMethod(env, NDKBridge, callbackGenericError, (*env)->NewStringUTF(env, txt));
			//__android_log_print(ANDROID_LOG_INFO, "M1Android", "Generic error.");

			break;
	}

	return 0;
}
jint Java_com_neko68k_M1_NDKBridge_getCurrentCmd(JNIEnv* env, jobject thiz){
	return(m1snd_get_info_int(M1_IINF_CURSONG, 0));
}

jstring Java_com_neko68k_M1_NDKBridge_getSongs(JNIEnv* env, jobject thiz, int song){

	int cmdNum = 0;
	int game =  m1snd_get_info_int(M1_IINF_CURGAME, 0);

	jstring track;


		cmdNum = m1snd_get_info_int(M1_IINF_TRACKCMD,(song<<16|game));
		//__android_log_print(ANDROID_LOG_INFO, "M1Android", "Cmd: %i", cmdNum);

		track = (*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_TRKNAME, (cmdNum<<16|game)));







	return(track);
}

jint Java_com_neko68k_M1_NDKBridge_getCurTime(JNIEnv* env, jobject thiz){
	return(m1snd_get_info_int(M1_IINF_CURTIME, 0));
}

jstring Java_com_neko68k_M1_NDKBridge_getMaker(JNIEnv* env, jobject thiz, jint parm){
	char * maker = m1snd_get_info_str(M1_SINF_MAKER, parm);
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Maker: %s", maker);
	return((*env)->NewStringUTF(env, maker));
}

jstring Java_com_neko68k_M1_NDKBridge_getBoard(JNIEnv* env, jobject thiz, jint parm){
	char *board = m1snd_get_info_str(M1_SINF_BNAME, m1snd_get_info_int(M1_IINF_BRDDRV, parm));
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Board: %s", board);
	return((*env)->NewStringUTF(env, board));
}

jstring Java_com_neko68k_M1_NDKBridge_getHardware(JNIEnv* env, jobject thiz, jint parm){
	char *hdw = m1snd_get_info_str(M1_SINF_BHARDWARE, m1snd_get_info_int(M1_IINF_BRDDRV, parm));
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Hardware: %s", hdw);
	return((*env)->NewStringUTF(env, hdw));
}

jint Java_com_neko68k_M1_NDKBridge_getMaxSongs(JNIEnv* env, jobject thiz){
	return(m1snd_get_info_int(M1_IINF_MAXSONG, 0));
}

jint Java_com_neko68k_M1_NDKBridge_nextSong(JNIEnv* env, jobject thiz){
	int current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	if (current < m1snd_get_info_int(M1_IINF_MAXSONG, 0))
	{
		m1snd_run(M1_CMD_SONGJMP, current+1);
		current += 1;
	}
	return current;
}
jint Java_com_neko68k_M1_NDKBridge_prevSong(JNIEnv* env, jobject thiz){
	int current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	if (current > m1snd_get_info_int(M1_IINF_MINSONG, 0))
	{
		m1snd_run(M1_CMD_SONGJMP, current-1);
		current -= 1;
	}
	return current;
}
void Java_com_neko68k_M1_NDKBridge_stop(JNIEnv* env, jobject thiz){
	m1snd_run(M1_CMD_STOP, 0);
}

void Java_com_neko68k_M1_NDKBridge_pause(JNIEnv* env, jobject thiz){
	m1snd_run(M1_CMD_PAUSE, 0);
}
void Java_com_neko68k_M1_NDKBridge_unPause(JNIEnv* env, jobject thiz){
	m1snd_run(M1_CMD_UNPAUSE, 0);
}



void Java_com_neko68k_M1_NDKBridge_waitForBoot(){
	waitForBoot();
}



jbyteArray Java_com_neko68k_M1_NDKBridge_m1sdrGenerationCallback(JNIEnv *env, jobject thiz){
	return(m1sdrGenerationCallback(env));
}

jstring Java_com_neko68k_M1_NDKBridge_getGameList(JNIEnv *env, jobject thiz, jint i){



	jstring returnString = (*env)->NewStringUTF(env, m1snd_get_info_str(M1_SINF_VISNAME, i));
	return(returnString);
}

jint Java_com_neko68k_M1_NDKBridge_getBootState(JNIEnv *env, jobject thiz){
	return booted;
}

jint Java_com_neko68k_M1_NDKBridge_getMaxGames(JNIEnv *env, jobject thiz){
	return max_games;
}

jint Java_com_neko68k_M1_NDKBridge_getROMID(JNIEnv *env, jobject thiz, jstring name){
	int i;
		int gotone = 0;
		char *str;
	     	str = (char*)(*env)->GetStringUTFChars(env, name, NULL);
		for (i = 0; i < max_games; i++)
		{
			if (!strcasecmp((char*)str, (char*)m1snd_get_info_str(M1_SINF_VISNAME, i)))
			{
				//m1snd_run(M1_CMD_GAMEJMP, i);
				return(i);
				gotone = 1;
				break;
			}
		}

		if (!gotone)
		{
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Unknown/unsupported game %s\n", (char*)str);
			return(-1);
		}
		return(i);
}

void Java_com_neko68k_M1_NDKBridge_nativeLoadROM(JNIEnv *env, jobject thiz, jint i){
	m1snd_run(M1_CMD_GAMEJMP, i);

}

void Java_com_neko68k_M1_NDKBridge_nativeClose(JNIEnv* env){
	m1snd_shutdown();
	free(rompath);
	free(wavpath);
}

void Java_com_neko68k_M1_M1Native_getPlayInfo(JNIEnv* env){
	/*int curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);

	current = m1snd_get_info_int(M1_IINF_CURCMD, 0);

	if (m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame) != -1)
	{
//				printf("time %d / %d\n", m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame), m1snd_get_info_int(M1_IINF_CURTIME, 0));
		if (m1snd_get_info_int(M1_IINF_CURTIME, 0) >= m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame))
		{
			ch = '+';	// cheat!
//					printf("next! %d %d\n", m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame), m1snd_get_info_int(M1_IINF_CURTIME, 0));
		}
	}*/
}

void Java_com_neko68k_M1_NDKBridge_jumpSong(JNIEnv* env, jobject thiz, int tracknum){
	m1snd_run(M1_CMD_SONGJMP, tracknum);
}

int Java_com_neko68k_M1_NDKBridge_getNumSongs( JNIEnv*  env, jobject thiz, int i){
	return(m1snd_get_info_int(M1_IINF_TRACKS, i));
}

// just check zip names to see if they exist, nothing fancy
//jstring Java_com_neko68k_M1_NDKBridge_simpleAudit( JNIEnv*  env, jobject thiz, int i){
void Java_com_neko68k_M1_NDKBridge_simpleAudit( JNIEnv*  env, jobject thiz, int i){

	char *zipName = m1snd_get_info_str(M1_SINF_ROMNAME, i);


	char *fullZipName = (char*)malloc(strlen(zipName)+5);
	sprintf(fullZipName, "%s.zip", zipName);
	//__android_log_print(ANDROID_LOG_INFO, "M1Android", "Checking...%s", fullZipName);
	jint test = i;
	//chdir("/sdcard/m1/roms/");
	chdir(rompath);
	FILE *testFile = fopen(fullZipName, "r");
	if(testFile!=NULL){
		fclose(testFile);

		//__android_log_print(ANDROID_LOG_INFO, "M1Android", "Found...%s", fullZipName);
		(*env)->CallStaticVoidMethod(env, NDKBridge, callbackROM, (*env)->NewStringUTF(env, m1snd_get_info_str(M1_SINF_VISNAME, i)), i);
		return;
		//free(fullZipName);

		//return((*env)->NewStringUTF(env, m1snd_get_info_str(M1_SINF_VISNAME, i)));
	}
	//__android_log_print(ANDROID_LOG_INFO, "M1Android", "ROM not found...%s", fullZipName);
	//return(NULL);
}

// verify rom crc's, maybe later
jstring Java_com_neko68k_M1_NDKBridge_CRCAudit( JNIEnv*  env, jobject thiz, int i){

}

// need to update this to take rompath and mixrate
void Java_com_neko68k_M1_NDKBridge_nativeInit( JNIEnv*  env, jobject thiz)
{
		FILE *cnf = NULL;
		m1snd_setoption(M1_OPT_RETRIGGER, 0);
		m1snd_setoption(M1_OPT_WAVELOG, 0);
		m1snd_setoption(M1_OPT_NORMALIZE, 1);
		m1snd_setoption(M1_OPT_LANGUAGE, M1_LANG_EN);
		m1snd_setoption(M1_OPT_RESETNORMALIZE, 0);
		m1snd_setoption(M1_OPT_SAMPLERATE, 44100);
		__android_log_print(ANDROID_LOG_INFO, "M1Android", "Starting init...");
		m1snd_init(NULL, m1ui_message);
		max_games = m1snd_get_info_int(M1_IINF_TOTALGAMES, 0);

		__android_log_print(ANDROID_LOG_INFO, "M1Android", "Max games: %i", max_games);
		//printf("\nM1: arcade video and pinball sound emu by R. Belmont\n");
		__android_log_print(ANDROID_LOG_INFO, "M1Android", "Core ver %s, CUI ver 2.4\n", m1snd_get_info_str(M1_SINF_COREVERSION, 0));
		//printf("Copyright (c) 2001-2010.  All Rights Reserved.\n\n");

		// this will be removed after the options are hooked up
		cnf = fopen("/sdcard/m1/m1.ini", "rt");
		if (!cnf)
		{
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "No config file found, using defaults\n");
			rompath = (char *)malloc(512);
			strcpy(rompath, "roms;");	// default rompath
			wavpath = (char *)malloc(512);
			strcpy(wavpath, "wave;");	// default wavepath
		}
		else
		{
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Reading configuration from m1.ini\n");
			rompath = (char *)malloc(65536);
			rompath[0] = '\0';
			wavpath = (char *)malloc(65536);
			wavpath[0] = '\0';
			read_config(cnf);
			fclose(cnf);
		}
	
}

// Quick MAME -listinfo compatible thing -pjp
// No info about parent roms if it isn't supported, therefore ignore
// Removes duplicate roms in same set
/*static void listinfo(void)
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
}*/
