/*
   m1ui.h - defines the interface between the user interface and 
            m1's core.

   Changes:

   M1 0.7.8: added SINF_GET_

   M1 0.7.5: added SINF_SET_ROMPATH and SINF_SET_WAVPATH to simplify .NET/Mono interfacing

   M1 0.7.3: removed internal shuffle mode.  added ability to disable internal sound driver.
             added option to not reset normalization state between songs.  added volume
	     controls.

 */

#ifndef _M1UI_H_
#define _M1UI_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __WIN32__
#define STDCALL	__stdcall
#define DLLEXPORT __attribute__ ((visibility("default")))
#else
#define STDCALL
#define DLLEXPORT
#endif

// M1 UI protocol version.  Check this to make sure you're OK.
#define M1UI_VERSION	(20)

// commands the UI passes to the m1 core (m1snd_run)
// these return 0 if successful and nonzero otherwise
enum
{
	M1_CMD_SONGJMP = 0,	// change song number
	M1_CMD_GAMEJMP,		// change game number
	M1_CMD_FLUSH,		// flush audio so it's safe to stop servicing idle...
	M1_CMD_STOP,		// stops playing and unloads game
	M1_CMD_PAUSE,		// pauses audio (no special effect if called recursively)
	M1_CMD_UNPAUSE,		// unpauses audio (ditto)

	M1_CMD_IDLE,		// idle time, must call at least 60 times/second

	M1_CMD_WRITELIST,	// write out a song's list data to an XML file.
				// iparm = the song number.
};

// options/parms the UI can set in the m1 core (m1snd_setoption)
enum
{
	M1_OPT_RETRIGGER=0,	// 0 for no auto-retrigger mode, 1 otherwise
	M1_OPT_DEFCMD,		// command # to override the default with
	M1_OPT_WAVELOG,		// 0 for no log to .WAV, 1 otherwise
	M1_OPT_NORMALIZE,	// enable/disable normalization
	M1_OPT_LANGUAGE,	// language for track list names
	M1_OPT_USELIST,		// set the ability to use track lists
	M1_OPT_INTERNALSND,	// set if M1 should use the system's audio output
	M1_OPT_SAMPLERATE,	// set the sample rate M1 runs at (default 44100, min 8000, max 48000)
	M1_OPT_RESETNORMALIZE,	// 0 = DON'T reset normalization state between songs (useful for albums), 1 = do reset it
	M1_OPT_FIXEDVOLUME,	// 0 = silence, 100 = regular volume, 300 = 3 times regular volume
	M1_OPT_POSTVOLUME,	// 0 = silence, 100 = volume as per previous stage (either normalize or OPT_FIXEDVOLUME).  Allows fadeouts.
				// IMPORTANT: the core sets this to 0 when a SONGJMP or GAMEJMP command is received, and 100 when the 
				// next song is about to start.  Thus, setting this between a song or game loading will have no effect.
	M1_OPT_STEREOMIX,	// amount of opposite channel mixing.  0 = full separation, 100 = full mono.
	M1_OPT_SAMPLES_PER_FRAME, // number of expected samples per frame.
};

// Languages to pass to M1_OPT_LANGUAGE
enum
{
	M1_LANG_EN=0,		// English
	M1_LANG_JP,		// Japanese
	M1_LANG_KO,		// Korean
	M1_LANG_CN,		// Chinese
	M1_LANG_ES,		// Spanish
	M1_LANG_FR,		// French
	M1_LANG_NL,		// Dutch
	M1_LANG_BE,		// Belgian
	M1_LANG_DE,		// German
	M1_LANG_PL,		// Polish
	M1_LANG_SE,		// Swedish
	M1_LANG_NO,		// Norwegian
	M1_LANG_FI,		// Finnish
	M1_LANG_RU,		// Russian
	M1_LANG_PT,		// Portugeuse (Portugal)
	M1_LANG_BR,		// Portugeuse (Brazil)

	M1_LANG_MAX		// (invalid)
};

// callback messages from the m1 core (m1ui_message)
enum
{
	M1_MSG_SWITCHING=0,	// switching games
	M1_MSG_GAMENAME,	// game name set
	M1_MSG_DRIVERNAME,	// driver name set
	M1_MSG_HARDWAREDESC,	// hardware description set
	M1_MSG_ROMLOADERR,	// rom load error!
	M1_MSG_STARTINGSONG,	// song # starting
	M1_MSG_BOOTING,		// board booting
	M1_MSG_BOOTFINISHED,	// board finished booting, ready for commands
	M1_MSG_SILENCE,		// there has been 2 seconds of silence
	M1_MSG_WAVEDATA,	// give the UI the current frame's wave data for VU/spectrum/etc
	M1_MSG_MATCHPATH,	// find the given filename in the current ROM search path
	M1_MSG_GETWAVPATH,	// return the path for .WAV logging
	M1_MSG_ERROR,		// show an error message
	M1_MSG_PAUSE,		// indicates pause/unpause (iparm = 0 when unpausing, 1 when pausing)
	M1_MSG_FLUSH,		// indicates you should flush any buffered audio
	M1_MSG_HARDWAREERROR,	// internal driver could not access the sound hardware
};

// numerical info the UI can request from the core (m1snd_get_info_int)
enum
{
	M1_IINF_HASPARENT=0,	// returns 0 if no parent romset, 1 otherwise
	M1_IINF_TOTALGAMES,	// returns the total # of supported games
	M1_IINF_CURSONG,	// returns the current song # (may not be command # in album mode!)
	M1_IINF_CURCMD,		// returns the current command #
	M1_IINF_CURGAME,	// returns the current game #
	M1_IINF_MINSONG,	// returns the lowest song # the current game supports
	M1_IINF_MAXSONG,	// returns the highest song # the current game supports
	M1_IINF_DEFSONG,	// returns the default song # for a game (parm = game number)
	M1_IINF_MAXDRVS,	// returns the total # of drivers
	M1_IINF_BRDDRV,		// returns the board number for a driver
	M1_IINF_ROMSIZE,	// returns the expected size of a ROM.  for "parm"
				// the low 16 bits is the game #, the top is the ROM #
	M1_IINF_ROMCRC,		// returns the expected CRC of a ROM.  parm is the
				// same as for M1_IINF_ROMSIZE.
	M1_IINF_ROMNUM,		// returns the number of ROMs for a game.  game # in parm.
	M1_IINF_TRACKS,		// returns the number of tracks (named songs) for a game.
	M1_IINF_TRKLENGTH,	// returns the length of time to play a track in 1/60 second units.
	                        // the low 16 bits is the game #, the top is the command number.
				// -1 is returned if no length is set in the .lst file.
	M1_IINF_TRACKCMD,	// returns the command number for a given track
	                        // the low 16 bits is the game #, the top is the track number.
	M1_IINF_CURTIME,	// returns the current time in the song, in 1/60th of a second units.
	M1_IINF_NUMEXTRAS,	// returns the number of extra text lines for a song.  param is the same
				// as for M1_IINF_TRKLENGTH, and -1 is returned if the game or song is not found.
	M1_IINF_NORMVOL,	// returns the current volume the normalization code has calculated (0-500+, where 100 = no amplify)
	M1_IINF_NUMSTREAMS,	// returns the current number of mixer streams (each stream has 1 or more channels)
	M1_IINF_NUMCHANS,	// returns the current number of mixer channels for a stream
	M1_IINF_CHANLEVEL,	// returns the level of a mixer channel (high 16 bits = stream #, low 16 bits = chan #)
	M1_IINF_CHANPAN,	// returns the pan setting of a mixer channel (high 16 bits = stream #, low 16 bits = chan #, result: 0 = center, 1 = left, 2 = right)
};

// string info types for UI/core communication
enum
{
	// basic commands, work with m1snd_get_info_str

	M1_SINF_ROMNAME = 0,	// returns the rom's .ZIP name
	M1_SINF_VISNAME,	// returns the full user-displayable game name
	M1_SINF_PARENTNAME,	// returns the parent set's .ZIP name
	M1_SINF_COREVERSION,	// returns the core version
	M1_SINF_MAKER,		// returns the manufacturer's name
	M1_SINF_BNAME,		// returns a board's name
	M1_SINF_BHARDWARE,	// returns a board's hardware description
	M1_SINF_ROMFNAME,	// returns a rom's expected filename.  for "parm",
	                        // the low 16 bits is the game #, the top is the ROM #
	M1_SINF_YEAR,		// returns the rom's year
	M1_SINF_TRKNAME,	// returns the name of a track in a given song.  for "parm",
	                        // the low 16 bits is the game #, the top is the track #
	M1_SINF_ENCODING,	// returns the ASCII name of the current languages' encoding
				// (format is suitable for use with GNU libiconv)
	M1_SINF_CHANNAME,	// returns the ASCII name of a mixer channel (hi 16 bits = stream, low 16 = channel)

	// extended commands. these are for use only with m1snd_get_info_str_ex!

	M1_SINF_EX_EXTRA,	// get "extra" text for a song, if any.  parm1 = the game, parm2 = the song,
				// and parm3 = the text item number

	// set commands.  these are for use only with m1snd_set_info_str!
	M1_SINF_SET_TRKNAME,	// set track's name.  info = name, parm1 = game, parm2 = song, parm3 = unused
	M1_SINF_SET_EXTRA,	// set "extra" text.  info = text parm1 = game, parm2 = song, parm3 = text item number
	M1_SINF_SET_ROMPATH,	// set pathname to find a ROM at.  do this when you get the M1_MSG_MATCHPATH callback.
	M1_SINF_SET_WAVPATH,	// set pathname to put .wavs at.  do this when you get the M1_MSG_GETWAVPATH callback.
};

// types for m1_set_info_int
enum
{
	M1_SIINF_CHANLEVEL,	// set the level of a mixer channel (parm1 = stream, parm2 = channel, parm3 = volume)
	M1_SIINF_CHANPAN,	// set the pan of a mixer channel (parm1 = stream, parm2 = channel, parm3 = pan (0=center, 1=left, 2=right)
};

// services m1snd defines for the use of the UI

// call this on startup to initialize the core.
DLLEXPORT void m1snd_init(void *, int (STDCALL *m1ui_message)(void *,int, char *, int));

// call this for "running" messages.
DLLEXPORT int m1snd_run(int command, int iparm);

// call this at program shutdown to shut down the core.
DLLEXPORT void m1snd_shutdown(void);

// call to set core options
DLLEXPORT void m1snd_setoption(int option, int value);

// call to get integer information from the core
DLLEXPORT int m1snd_get_info_int(int iinfo, int parm);

// call to get string information from the core
DLLEXPORT char *m1snd_get_info_str(int sinfo, int parm);

// call to get extended string info
DLLEXPORT char *m1snd_get_info_str_ex(int sinfo, int parm1, int parm2, int parm3);

// call to set string data
DLLEXPORT void m1snd_set_info_str(int sinfo, char *info, int parm1, int parm2, int parm3);

// call to set integer information in the core
DLLEXPORT void m1snd_set_info_int(int iinfo, int parm1, int parm2, int parm3);

// call when not using M1's internal sound driver to create a frame of output
DLLEXPORT void m1snd_do_frame(unsigned long dwSamples, signed short *out);

#ifdef __cplusplus
}
#endif

#endif
