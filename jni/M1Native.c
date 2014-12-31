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

static jclass NDKBridge;
static jclass Game;
static jmethodID callbackSilence;
static jmethodID callbackLoadError;
static jmethodID callbackGenericError;
//static JNIEnv *cbEnv;
static JavaVM *VM;

jint JNI_OnLoad(JavaVM* vm, void* reserved){
	JNIEnv *env;
	    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK)
	        return -1;
	    VM = vm;
	    /* get class with (*env)->FindClass */
	    /* register methods with (*env)->RegisterNatives */


	//cbEnv = env;
	jclass lNDKBridge = (*env)->FindClass(env, "com/neko68k/M1/NDKBridge");
	NDKBridge = (*env)->NewGlobalRef(env, lNDKBridge);
	callbackLoadError = (*env)->GetStaticMethodID(env, NDKBridge, "RomLoadErr", "()V");
	callbackGenericError = (*env)->GetStaticMethodID(env, NDKBridge, "GenericError", "(Ljava/lang/String;)V");
	callbackSilence = (*env)->GetStaticMethodID(env, NDKBridge, "Silence", "()V");

	jclass tmp = (*env)->FindClass(env, "com/neko68k/M1/Game");
	Game = (jclass)(*env)->NewGlobalRef(env, tmp);

    return JNI_VERSION_1_6;
}

// Public functions

// callbacks from the core of interest to the user interface
// this will make calls to Java statics, this !!must!! be set up
// in the init code!!!
static int m1ui_message(void *this, int message, char *txt, int iparm)
{
	JNIEnv *env;
	int curgame;
	(*VM)->GetEnv(VM, (void**) &env, JNI_VERSION_1_6);
	switch (message)
	{
		// called when switching to a new game
		case M1_MSG_SWITCHING:
			
			break;

		// called to show the current game's name
		case M1_MSG_GAMENAME:
			curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			//jstring retstring = (*env)->NewStringUTF(env, m1snd_get_info_str(M1_SINF_VISNAME, curgame));
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Game: %i %s", curgame, m1snd_get_info_str(M1_SINF_VISNAME, curgame));

			//(*env)->CallStaticVoidMethod(env, NDKBridge, callbackSetGameName, retstring);
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

int Java_com_neko68k_M1_NDKBridge_getInfoInt(JNIEnv* env, jobject thiz, int cmd, int parm){
	return(m1snd_get_info_int(cmd, parm));
}

jstring Java_com_neko68k_M1_NDKBridge_getInfoStr(JNIEnv* env, jobject thiz, int cmd, int parm){
	return((*env)->NewStringUTF(env, m1snd_get_info_str(cmd, parm)));
}

/*jstring Java_com_neko68k_M1_NDKBridge_getInfoStrEX(JNIEnv* env, jobject thiz, int cmd, int parm){
	return((*env)->NewStringUTF(env, m1snd_get_info_str_ex(cmd, parm)));
}*/
jobject Java_com_neko68k_M1_NDKBridge_queryRom(JNIEnv* env, jobject thiz, int game){
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Getting 'Game' class in native...");
	jmethodID constructor = (*env)->GetMethodID(env, Game, "<init>", "()V"); //The name of constructor method is "<init>"
	jobject instance = (*env)->NewObject(env, Game, constructor);


	//jint audit =;
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Reading %i: %s\n", game, m1snd_get_info_str(M1_SINF_VISNAME, game));

	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, Game, "title", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_VISNAME, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, Game, "romname", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_ROMNAME, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, Game, "mfg", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_MAKER, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, Game, "sys", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_BNAME, m1snd_get_info_int(M1_IINF_BRDDRV, game))));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, Game, "year", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_YEAR, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, Game, "cpu", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_BHARDWARE, m1snd_get_info_int(M1_IINF_BRDDRV, game))));
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Finished %i: %s\n", game, m1snd_get_info_str(M1_SINF_VISNAME, game));
	//(*env)->SetIntField(env, instance, (*env)->GetFieldID(env, Game, "romavail", "I"),
		//	 1);
	/*(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, complexClass, "title", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_VISNAME, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, complexClass, "title", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_VISNAME, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, complexClass, "title", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_VISNAME, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, complexClass, "title", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_VISNAME, game)));
	(*env)->SetObjectField(env, instance, (*env)->GetFieldID(env, complexClass, "title", "Ljava/lang/String;"),
				(*env)->NewStringUTF(env, (char*)m1snd_get_info_str(M1_SINF_VISNAME, game)));

*/
	//__android_log_print(ANDROID_LOG_INFO, "M1Android", "Done.\n");
	return instance;
}

int Java_com_neko68k_M1_NDKBridge_getCurTime(JNIEnv* env, jobject thiz){
	return(m1snd_get_info_int(M1_IINF_CURTIME, 0));
}

int Java_com_neko68k_M1_NDKBridge_getMaxSongs(JNIEnv* env, jobject thiz){
	return(m1snd_get_info_int(M1_IINF_MAXSONG, 0));
}

int Java_com_neko68k_M1_NDKBridge_nextSong(JNIEnv* env, jobject thiz){
	int current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	if (current < m1snd_get_info_int(M1_IINF_MAXSONG, 0))
	{
		m1snd_run(M1_CMD_SONGJMP, current+1);
		current += 1;
	}
	return current;
}
int Java_com_neko68k_M1_NDKBridge_prevSong(JNIEnv* env, jobject thiz){
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

int Java_com_neko68k_M1_NDKBridge_GetSongLen(JNIEnv* env, jobject thiz){
//	M1_IINF_TRKLENGTH
// returns the length of time to play a track in 1/60 second units.
// the low 16 bits is the game #, the top is the command number.
// -1 is returned if no length is set in the .lst file.
	int game = m1snd_get_info_int(M1_IINF_CURGAME, 0);
	int cmd = m1snd_get_info_int(M1_IINF_CURCMD, 0);
	int optcmd = cmd<<16|game;
	return m1snd_get_info_int(M1_IINF_TRKLENGTH, optcmd);

}

void Java_com_neko68k_M1_NDKBridge_waitForBoot(){
	waitForBoot();
}

jbyteArray Java_com_neko68k_M1_NDKBridge_m1sdrGenerationCallback(JNIEnv *env, jobject thiz){
	return(m1sdrGenerationCallback(env));
}
int Java_com_neko68k_M1_NDKBridge_getBootState(JNIEnv *env, jobject thiz){
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Booted.");
	return booted;
}

int Java_com_neko68k_M1_NDKBridge_getMaxGames(JNIEnv *env, jobject thiz){
	return max_games;
}

void Java_com_neko68k_M1_NDKBridge_nativeLoadROM(JNIEnv *env, jobject thiz, jint i){
	m1snd_run(M1_CMD_GAMEJMP, i);

}

void Java_com_neko68k_M1_NDKBridge_nativeClose(JNIEnv* env){
	m1snd_shutdown();
	free(rompath);
	free(wavpath);
}

void Java_com_neko68k_M1_NDKBridge_jumpSong(JNIEnv* env, jobject thiz, int tracknum){
	m1snd_run(M1_CMD_SONGJMP, tracknum);
}

void Java_com_neko68k_M1_NDKBridge_restSong(JNIEnv* env, jobject thiz){
	int cursong = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	m1snd_run(M1_CMD_SONGJMP, cursong);
}

int Java_com_neko68k_M1_NDKBridge_getNumSongs( JNIEnv*  env, jobject thiz, int i){
	int numsongs = m1snd_get_info_int(M1_IINF_TRACKS, i);
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Num songs: %i\n", numsongs);
	return(numsongs);
}

int Java_com_neko68k_M1_NDKBridge_simpleAudit(JNIEnv* env, jobject thiz, int i){
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Auditing %i.\n", i);
	char *zipName = m1snd_get_info_str(M1_SINF_ROMNAME, i);

	char *fullZipName = (char*)malloc(strlen(zipName)+5);
	sprintf(fullZipName, "%s.zip", zipName);
	chdir(rompath);
	FILE *testFile = fopen(fullZipName, "r");
	if(testFile!=NULL){
		fclose(testFile);
		free(fullZipName);
		__android_log_print(ANDROID_LOG_INFO, "M1Android", "%s OK!\n", zipName);
		return 1;
	}
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "%s NOT-OK!\n", zipName);
	return 0;
}

void Java_com_neko68k_M1_NDKBridge_SetOption( JNIEnv*  env, jobject thiz, int opt, int val){
	int vali = val;
	m1snd_setoption(opt, vali);
	__android_log_print(ANDROID_LOG_INFO, "M1Android", "Setting options...\n");
}

// TODO: fix this shit up to take a jstring arg that is the rompath
void Java_com_neko68k_M1_NDKBridge_nativeInit( JNIEnv*  env, jobject thiz, jstring basepath)
{
		FILE *cnf = NULL;
		m1snd_setoption(M1_OPT_RETRIGGER, 0);
		m1snd_setoption(M1_OPT_WAVELOG, 0);
		//m1snd_setoption(M1_OPT_NORMALIZE, 1);
//		m1snd_setoption(M1_OPT_LANGUAGE, M1_LANG_EN);
		//m1snd_setoption(M1_OPT_RESETNORMALIZE, 0);
		m1snd_setoption(M1_OPT_SAMPLERATE, 44100);
		__android_log_print(ANDROID_LOG_INFO, "M1Android", "Starting init...");
		const char *nbasepath = (*env)->GetStringUTFChars(env, basepath, 0);
		m1snd_init(NULL, m1ui_message, nbasepath);
		max_games = m1snd_get_info_int(M1_IINF_TOTALGAMES, 0);

		__android_log_print(ANDROID_LOG_INFO, "M1Android", "Max games: %i", max_games);
		//printf("\nM1: arcade video and pinball sound emu by R. Belmont\n");
		__android_log_print(ANDROID_LOG_INFO, "M1Android", "Core ver %s, CUI ver 2.4\n", m1snd_get_info_str(M1_SINF_COREVERSION, 0));
		//printf("Copyright (c) 2001-2010.  All Rights Reserved.\n\n");

		// this will be removed after the options are hooked up
		

		/*cnf = fopen("/sdcard/m1/m1.ini", "rt");
		if (!cnf)
		{*/
			
			


			// use your string

			
			
			rompath = (char *)malloc(512);
			sprintf(rompath, "%s", nbasepath);
//			strcpy(rompath, "/sdcard/m1/roms;");	// default rompath
			wavpath = (char *)malloc(512);
			sprintf(wavpath, "%s/m1/wave;", nbasepath);
			//strcpy(wavpath, "/sdcard/m1/wave;");	// default wavepath
			(*env)->ReleaseStringUTFChars(env, basepath, nbasepath);
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Init done.\n");
		/*}
		else
		{
			__android_log_print(ANDROID_LOG_INFO, "M1Android", "Reading configuration from m1.ini\n");
			rompath = (char *)malloc(65536);
			rompath[0] = '\0';
			rompathshort = (char *)malloc(65536);
			rompathshort[0] = '\0';
			wavpath = (char *)malloc(65536);
			wavpath[0] = '\0';
			read_config(cnf);
			fclose(cnf);
		}*/
	
}


