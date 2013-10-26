package com.neko68k.M1;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.widget.TextView;
import android.widget.Toast;




// this implements all the stuff we
// need for getting the native
// stuff up and running

// basically this will have some statics
// for play control and for init/deinit
// of the engine on the JNI side








// this is all the Java to native stuff
public class NDKBridge {
	/////////////////////// ENUMS
	static final int M1_OPT_RETRIGGER=0;	// 0 for no auto-retrigger mode, 1 otherwise
	static final int M1_OPT_DEFCMD = 1;		// command # to override the default with
	static final int M1_OPT_WAVELOG =2;		// 0 for no log to .WAV, 1 otherwise
	static final int M1_OPT_NORMALIZE =3;	// enable/disable normalization
	static final int M1_OPT_LANGUAGE =4;	// language for track list names
	static final int M1_OPT_USELIST =5;		// set the ability to use track lists
	static final int M1_OPT_INTERNALSND =6; // set if M1 should use the system's audio output
	static final int M1_OPT_SAMPLERATE =7;	// set the sample rate M1 runs at (default 44100, min 8000, max 48000)
	static final int M1_OPT_RESETNORMALIZE=8;	// 0 = DON'T reset normalization state between songs (useful for albums), 1 = do reset it
	static final int M1_OPT_FIXEDVOLUME=9;	// 0 = silence, 100 = regular volume, 300 = 3 times regular volume
	static final int M1_OPT_POSTVOLUME=10;
	
	
	// types for m1_get_info_str/_ex
	static final int M1_SINF_ROMNAME = 0;	// returns the rom's .ZIP name
	static final int M1_SINF_VISNAME=1;		// returns the full user-displayable game name
	static final int M1_SINF_PARENTNAME=2;	// returns the parent set's .ZIP name
	static final int M1_SINF_COREVERSION=3;	// returns the core version
	static final int M1_SINF_MAKER=4;		// returns the manufacturer's name
	static final int M1_SINF_BNAME=5;		// returns a board's name
	static final int M1_SINF_BHARDWARE=6;	// returns a board's hardware description
	static final int M1_SINF_ROMFNAME=7;	// returns a rom's expected filename.  for "parm"=0;
		                        			// the low 16 bits is the game #=0; the top is the ROM #
	static final int M1_SINF_YEAR=8;		// returns the rom's year
	static final int M1_SINF_TRKNAME=9;		// returns the name of a track in a given song.  for "parm"=0;
		                        			// the low 16 bits is the game #=0; the top is the track #
	static final int M1_SINF_ENCODING=10;	// returns the ASCII name of the current languages' encoding
											// (format is suitable for use with GNU libiconv)
	static final int M1_SINF_CHANNAME=11;	// returns the ASCII name of a mixer channel (hi 16 bits = stream=0; low 16 = channel)
											// extended commands. these are for use only with m1snd_get_info_str_ex!
	static final int M1_SINF_EX_EXTRA=12;	// get "extra" text for a song=0; if any.  parm1 = the game=0; parm2 = the song=0;
											// and parm3 = the text item number
											// set commands.  these are for use only with m1snd_set_info_str!
	static final int M1_SINF_SET_TRKNAME=13;// set track's name.  info = name=0; parm1 = game=0; parm2 = song=0; parm3 = unused
	static final int M1_SINF_SET_EXTRA=14;	// set "extra" text.  info = text parm1 = game=0; parm2 = song=0; parm3 = text item number
	static final int M1_SINF_SET_ROMPATH=15;// set pathname to find a ROM at.  do this when you get the M1_MSG_MATCHPATH callback.
	static final int M1_SINF_SET_WAVPATH=16;// set pathname to put .wavs at.  do this when you get the M1_MSG_GETWAVPATH callback.

	
	// types for m1_set_info_int

	static final int M1_SIINF_CHANLEVEL=0;	// set the level of a mixer channel (parm1 = stream, parm2 = channel, parm3 = volume)
	static final int M1_SIINF_CHANPAN=1;	// set the pan of a mixer channel (parm1 = stream, parm2 = channel, parm3 = pan (0=center, 1=left, 2=right)

	//////////////////////////// END ENUMS
	
	static GameListOpenHelper db;
	
	static int defLen;
	static int songLen;
	static boolean inited = false;
	static int playtime;
	static Integer curGame;
	static String board;
	static String hdw;
	static String mfg;
	
	static int mixrate;
		
	static String xmlPath;
	static String listPath;
	static String romPath;
	
	static String basepath = null;
	
	static GameListAdapter globalGLA = new GameListAdapter();
	static Map<String, Integer> lookup = new HashMap<String, Integer>();
	static GameListOpenHelper m1db;
	//Map<String, Integer> m = new HashMap<String, Integer>();
	
	static TextView Title;
	static TextView PlayTime;
	static TextView TrackNum;
	static boolean loadError = false;
	//static GameListAdapter nonglobalgla = new GameListAdapter();	
	static PlayerService playerService = new PlayerService();
	static String m1error;
	static Context ctx;
	static int cur;
	static public void setTitleView(TextView tv){
		Title = tv;
	}

	public static void SetGameName(String name){
		Title.setText(name);
	}
	
	 

	public static void addROM(String name, Integer id){
		if(name!=null){
			GameList it = new GameList(name);
			NDKBridge.lookup.put(name, new Integer(cur));
			//Log.v("M1Android", "Adding item");
			globalGLA.addItem(it);
		}
	}
	public static void RomLoadErr(){
		// this flag prevents it from attempting to start playing
		// if the rom didn't load
		loadError = true;		
	}

	public static void GenericError(String instring){
		// pop a notification that says something bad happened
		m1error = instring;
		Toast.makeText(ctx, NDKBridge.m1error, Toast.LENGTH_SHORT).show();
	}
	public static void Silence(){
		NDKBridge.next();
		//playerService.setNoteText();

		// if we are to skip songs when we here silence, this is where we do it
	}
	
	public static int next(){
		playtime = 0;
		int i = nextSong();
		playerService.setNoteText();
		return i;
	}
	public static void initM1(){
    	nativeInit(basepath);		      	
	}		
	public static Game queryROM(int i){
		Game game = new Game();
		String bhardware;
		game.setIndex(i);
		game.setTitle(getInfoString(M1_SINF_VISNAME, i));
		game.setYear(getInfoString(M1_SINF_YEAR, i));
		game.setRomname(getInfoString(M1_SINF_ROMFNAME, i));
		game.setMfg(getInfoString(M1_SINF_MAKER, i));
		game.setSys(getInfoString(M1_SINF_BNAME, i));
		bhardware = getInfoString(M1_SINF_BHARDWARE, i);
		List<String> hardwareitems = Arrays.asList(bhardware.split("\\s*,\\s*"));
		switch(hardwareitems.size()){
		case 4:
			game.setSound4(hardwareitems.get(4));
		case 3:
			game.setSound3(hardwareitems.get(3));
		case 2:
			game.setSound2(hardwareitems.get(2));
		case 1:
			game.setSound1(hardwareitems.get(1));
		case 0:
			game.setCpu(hardwareitems.get(0));
			break;
		}
		game.setListavail(0);
		return game;
	}
	public static void queryROM(Game game, String name){
		
	}
	public static void loadROM(String name){
		
		//curGame = getROMID(name);
		curGame =(Integer)lookup.get(name);
		board = getBoard(curGame);
		hdw = getHardware(curGame);
		mfg = getMaker(curGame);
		nativeLoadROM(curGame);
	}
	public static GameList getGameTitle(int i){
		GameList entry = new GameList(getGameList(i));		
		return(entry);
	}
	public static native int getMaxGames();
    public static native String getGameList(int i);
    
    public static native void nativeInit(String basepath);
    //public static native void initCallbacks();
    public static native void nativeClose();
    public static String auditROM(int i){
    	return(simpleAudit(i));
    }
    public static native String simpleAudit(int i);
    
    public static native Integer getInfoInteger(int cmd, int parm);
    public static native String getInfoString(int cmd, int parm);
    public static native String getInfoStringEx(int cmd, int parm);
    
    public static native int getCurrentCmd();
    public static native int getNumSongs(int gameNum);
    public static native int getMaxSongs();
    public static native String getSongs(int song);    
    public static String getSong(int i){
    	return(getSongs(i));
    }
    
    public static native int nextSong();
    public static native int prevSong();
    public static native void stop();
    public static native void pause();
    public static native void unPause();
    public static native void restSong();
    
    public static native int getCurTime();
    public static native String getMaker(int parm);
    public static native String getBoard(int parm);
    public static native String getHardware(int parm);
    
    public static native int nativeLoadROM(int i);
    public static native int getROMID(String name);
    public static native byte[] m1sdrGenerationCallback();
    public static native int getBootState();
    public static native void waitForBoot();  
    public static native void jumpSong(int i);
    public static native void SetOption(int opt, int val);
    public static native int GetSongLen();
    
    public static void getSongLen(){
    	songLen = GetSongLen();
    	if(songLen==-1)
    		songLen = defLen;    	
    	else 
    		songLen = songLen/60;
    }

    
    static {
        System.loadLibrary("M1");
    }
}



