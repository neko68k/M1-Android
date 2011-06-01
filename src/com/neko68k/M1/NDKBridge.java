package com.neko68k.M1;

import java.util.HashMap;
import java.util.Map;




// this implements all the stuff we
// need for getting the native
// stuff up and running

// basically this will have some statics
// for play control and for init/deinit
// of the engine on the JNI side

// this is all the Java to native stuff
public class NDKBridge {
	static boolean inited = false;
	static int playtime;
	static Integer curGame;
	static String board;
	static String hdw;
	static String mfg;
	
	static int mixrate;
	static String romPath;
	
	static GameListAdapter globalGLA;
	static Map<String, Integer> lookup = new HashMap<String, Integer>();
	//Map<String, Integer> m = new HashMap<String, Integer>();
	
	public static int next(){
		playtime = 0;
		return(nextSong());
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
    
    static {
        System.loadLibrary("M1");
    }
}



