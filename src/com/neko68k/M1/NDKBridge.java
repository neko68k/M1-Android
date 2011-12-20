package com.neko68k.M1;

import java.util.HashMap;
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
	
	static int defLen;
	static int songLen;
	static boolean inited = false;
	static int playtime;
	static Integer curGame;
	static String board;
	static String hdw;
	static String mfg;
	
	static int mixrate;
	static String romPath;
	
	static GameListAdapter globalGLA = new GameListAdapter();
	static Map<String, Integer> lookup = new HashMap<String, Integer>();
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
    	nativeInit();		      	
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
    
    public static native void nativeInit();
    //public static native void initCallbacks();
    public static native void nativeClose();
    public static String auditROM(int i){
    	return(simpleAudit(i));
    }
    public static native String simpleAudit(int i);
    
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



