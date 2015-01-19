package com.neko68k.M1;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

// this implements all the stuff we
// need for getting the native
// stuff up and running

// basically this will have some statics
// for play control and for init/deinit
// of the engine on the JNI side

// this is all the Java to native stuff
public class NDKBridge {
	// ///////////////////// ENUMS
	static final int M1_OPT_RETRIGGER = 0; // 0 for no auto-retrigger mode, 1
											// otherwise
	static final int M1_OPT_DEFCMD = 1; // command # to override the default
										// with
	static final int M1_OPT_WAVELOG = 2; // 0 for no log to .WAV, 1 otherwise
	static final int M1_OPT_NORMALIZE = 3; // enable/disable normalization
	static final int M1_OPT_LANGUAGE = 4; // language for track list names
	static final int M1_OPT_USELIST = 5; // set the ability to use track lists
	static final int M1_OPT_INTERNALSND = 6; // set if M1 should use the
												// system's audio output
	static final int M1_OPT_SAMPLERATE = 7; // set the sample rate M1 runs at
											// (default 44100, min 8000, max
											// 48000)
	static final int M1_OPT_RESETNORMALIZE = 8; // 0 = DON'T reset normalization
												// state between songs (useful
												// for albums), 1 = do reset it
	static final int M1_OPT_FIXEDVOLUME = 9; // 0 = silence, 100 = regular
												// volume, 300 = 3 times regular
												// volume
	//NDKBridge.SetOption(NDKBridge.M1_OPT_FIXEDVOLUME, 50);
	static final int M1_OPT_POSTVOLUME = 10;

	// types for m1_get_info_str/_ex
	static final int M1_SINF_ROMNAME = 0; // returns the rom's .ZIP name
	static final int M1_SINF_VISNAME = 1; // returns the full user-displayable
											// game name
	static final int M1_SINF_PARENTNAME = 2; // returns the parent set's .ZIP
												// name
	static final int M1_SINF_COREVERSION = 3; // returns the core version
	static final int M1_SINF_MAKER = 4; // returns the manufacturer's name
	static final int M1_SINF_BNAME = 5; // returns a board's name
	static final int M1_SINF_BHARDWARE = 6; // returns a board's hardware
											// description
	static final int M1_SINF_ROMFNAME = 7; // returns a rom's expected filename.
											// for "parm"=0;
											// the low 16 bits is the game #=0;
											// the top is the ROM #
	static final int M1_SINF_YEAR = 8; // returns the rom's year
	static final int M1_SINF_TRKNAME = 9; // returns the name of a track in a
											// given song. for "parm"=0;
											// the low 16 bits is the game #=0;
											// the top is the track #
	static final int M1_SINF_ENCODING = 10; // returns the ASCII name of the
											// current languages' encoding
											// (format is suitable for use with
											// GNU libiconv)
	static final int M1_SINF_CHANNAME = 11; // returns the ASCII name of a mixer
											// channel (hi 16 bits = stream=0;
											// low 16 = channel)
											// extended commands. these are for
											// use only with
											// m1snd_get_info_str_ex!
	static final int M1_SINF_EX_EXTRA = 12; // get "extra" text for a song=0; if
											// any. parm1 = the game=0; parm2 =
											// the song=0;
											// and parm3 = the text item number
											// set commands. these are for use
											// only with m1snd_set_info_str!
	static final int M1_SINF_SET_TRKNAME = 13;// set track's name. info =
												// name=0; parm1 = game=0; parm2
												// = song=0; parm3 = unused
	static final int M1_SINF_SET_EXTRA = 14; // set "extra" text. info = text
												// parm1 = game=0; parm2 =
												// song=0; parm3 = text item
												// number
	static final int M1_SINF_SET_ROMPATH = 15;// set pathname to find a ROM at.
												// do this when you get the
												// M1_MSG_MATCHPATH callback.
	static final int M1_SINF_SET_WAVPATH = 16;// set pathname to put .wavs at.
												// do this when you get the
												// M1_MSG_GETWAVPATH callback.

	// types for m1_set_info_int

	static final int M1_IINF_HASPARENT = 0; // returns 0 if no parent romset, 1
											// otherwise
	static final int M1_IINF_TOTALGAMES = 1; // returns the total # of supported
												// games
	static final int M1_IINF_CURSONG = 2; // returns the current song # (may not
											// be command # in album mode!)
	static final int M1_IINF_CURCMD = 3; // returns the current command #
	static final int M1_IINF_CURGAME = 4; // returns the current game #
	static final int M1_IINF_MINSONG = 5; // returns the lowest song # the
											// current game supports
	static final int M1_IINF_MAXSONG = 6; // returns the highest song # the
											// current game supports
	static final int M1_IINF_DEFSONG = 7; // returns the default song # for a
											// game (parm = game number)
	static final int M1_IINF_MAXDRVS = 8; // returns the total # of drivers
	static final int M1_IINF_BRDDRV = 9; // returns the board number for a
											// driver
	static final int M1_IINF_ROMSIZE = 10; // returns the expected size of a
											// ROM. for "parm"
	// the low 16 bits is the game #, the top is the ROM #
	static final int M1_IINF_ROMCRC = 11; // returns the expected CRC of a ROM.
											// parm is the
	// same as for M1_IINF_ROMSIZE.
	static final int M1_IINF_ROMNUM = 12; // returns the number of ROMs for a
											// game. game # in parm.
	static final int M1_IINF_TRACKS = 13; // returns the number of tracks (named
											// songs) for a game.
	static final int M1_IINF_TRKLENGTH = 14; // returns the length of time to
												// play a track in 1/60 second
												// units.
	// the low 16 bits is the game #, the top is the command number.
	// -1 is returned if no length is set in the .lst file.
	static final int M1_IINF_TRACKCMD = 15; // returns the command number for a
											// given track
	// the low 16 bits is the game #, the top is the track number.
	static final int M1_IINF_CURTIME = 16; // returns the current time in the
											// song, in 1/60th of a second
											// units.
	static final int M1_IINF_NUMEXTRAS = 16; // returns the number of extra text
												// lines for a song. param is
												// the same
	// as for M1_IINF_TRKLENGTH, and -1 is returned if the game or song is not
	// found.
	static final int M1_IINF_NORMVOL = 17; // returns the current volume the
											// normalization code has calculated
											// (0-500+, where 100 = no amplify)
	static final int M1_IINF_NUMSTREAMS = 18; // returns the current number of
												// mixer streams (each stream
												// has 1 or more channels)
	static final int M1_IINF_NUMCHANS = 19; // returns the current number of
											// mixer channels for a stream
	static final int M1_IINF_CHANLEVEL = 20; // returns the level of a mixer
												// channel (high 16 bits =
												// stream #, low 16 bits = chan
												// #)
	static final int M1_IINF_CHANPAN = 21; // returns the pan setting of a mixer
											// channel (high 16 bits = stream #,
											// low 16 bits = chan #, result: 0 =
											// center, 1 = left, 2 = right)

	// ////////////////////////// END ENUMS

	static GameDatabaseHelper m1db;
	static Game game = null;
	static int defLen;
	static int songLen;
	static boolean inited = false;
	static int playtime;
	static int mixrate;
	static String xmlPath;
	static String listPath;
	static String rompath = null;
	static String iconpath = null;
	static String basepath = null;
	static boolean loadError = false;
	static PlayerService playerService = new PlayerService();
	static String m1error;
	static Context ctx;
	static int cur = -1;
	static Boolean forceRescan = false;

	public static void RomLoadErr() {
		// this flag prevents it from attempting to start playing
		// if the rom didn't load
		loadError = true;
	}

	public static void GenericError(String instring) {
		// pop a notification that says something bad happened
		m1error = instring;
        //Error: No games or couldn't find m1.xml!
        if(!m1error.contentEquals("Error: No games or couldn't find m1.xml!")) {
            Toast.makeText(ctx, NDKBridge.m1error, Toast.LENGTH_SHORT).show();
        }
	}

	public static void Silence() {
		//ctx.startActivity(new Intent(PlayerService.ACTION_SKIP, null, ctx, PlayerService.class));
        ctx.startService(new Intent(PlayerService.ACTION_SKIP, null, ctx.getApplicationContext(), PlayerService.class));
		// playerService.setNoteText();

		// if we are to skip songs when we here silence, this is where we do it
	}

	public static int next() {
		playtime = 0;
		int i = nextSong();
		//playerService.setNoteText();
		
		return i;
	}

	public static void initM1() {
		nativeInit(basepath, rompath);
	}

	public static Bitmap getIcon() {
		Bitmap bm = null;
		// v = params[0];

		File file = new File(iconpath
				+ game.romname + ".ico");
		FileInputStream inputStream;
		try {
			inputStream = new FileInputStream(file);
			bm = BitmapFactory.decodeStream(inputStream);
			inputStream.close();
		} catch (FileNotFoundException e) {
			// check for parent(and clone) romsets
			// if none set default icon
			file = new File(iconpath
					+ getInfoStr(NDKBridge.M1_SINF_PARENTNAME, cur)
					+ ".ico");
			try {
				inputStream = new FileInputStream(file);
				bm = BitmapFactory.decodeStream(inputStream);
				inputStream.close();

			} catch (FileNotFoundException e1) {
				// TODO Auto-generated catch block
				file = new File(iconpath+"!MAMu_0.145u4.ico");
				try {
					inputStream = new FileInputStream(file);
					bm = BitmapFactory.decodeStream(inputStream);
					inputStream.close();

				} catch (FileNotFoundException e2) {
					// TODO Auto-generated catch block

					e2.printStackTrace();
					return (null);
				} catch (IOException e2) {
					// TODO Auto-generated catch block
					e2.printStackTrace();
				}
				//return (null);
			} catch (IOException e2) {
				// TODO Auto-generated catch block
				e2.printStackTrace();
			}

			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return (Bitmap.createScaledBitmap(bm, 128, 128, false));
	}
	
	public static Bitmap getIcon(String romname, int romnum) {
		Bitmap bm = null;
		// v = params[0];

		File file = new File(iconpath
				+ romname + ".ico");
		FileInputStream inputStream;
		try {
			inputStream = new FileInputStream(file);
			bm = BitmapFactory.decodeStream(inputStream);
			inputStream.close();
		} catch (FileNotFoundException e) {
			// check for parent(and clone) romsets
			// if none set default icon
			file = new File(iconpath
					+ getInfoStr(NDKBridge.M1_SINF_PARENTNAME, romnum)
					+ ".ico");
			try {
				inputStream = new FileInputStream(file);
				bm = BitmapFactory.decodeStream(inputStream);
				inputStream.close();

			} catch (FileNotFoundException e1) {
				// TODO Auto-generated catch block
				file = new File(iconpath+"!MAMu_0.145u4.ico");
				try {
					inputStream = new FileInputStream(file);
					bm = BitmapFactory.decodeStream(inputStream);
					inputStream.close();

				} catch (FileNotFoundException e2) {
					// TODO Auto-generated catch block

					e2.printStackTrace();
					return (null);
				} catch (IOException e2) {
					// TODO Auto-generated catch block
					e2.printStackTrace();
				}
				//return (null);
			} catch (IOException e2) {
				// TODO Auto-generated catch block
				e2.printStackTrace();
			}

			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return (Bitmap.createScaledBitmap(bm, 128, 128, false));
	}

	public static native void nativeInit(String basepath, String rompath);

	public static native void nativeClose();

	public static native int simpleAudit(int i);

	public static native int getInfoInt(int cmd, int parm);

	public static native String getInfoStr(int cmd, int parm);

	public static native String getInfoStringEx(int cmd, int parm);

	public static native Game queryRom(int i);

	public static native int getMaxGames();

	public static native int nextSong();

	public static native int prevSong();

	public static native void stop();

	public static native void pause();

	public static native void unPause();

	public static native void restSong();

	public static native int getCurTime();

	public static native int nativeLoadROM(int i);

	public static native int getROMID(String name);

	public static native byte[] m1sdrGenerationCallback();

	public static native int getBootState();

	public static native void waitForBoot();

	public static native void jumpSong(int i);

	public static native void SetOption(int opt, int val);

	public static native int GetSongLen();

	public static void getSongLen() {
		songLen = GetSongLen();
		if (songLen == -1)
			songLen = defLen;
		else
			songLen = songLen / 60;
	}

	static {
		System.loadLibrary("M1");
	}
}
