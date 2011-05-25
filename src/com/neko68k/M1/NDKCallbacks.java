package com.neko68k.M1;

import android.content.Context;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;


// this will implement all the stuff that
// we need to have the Native code
// call directly (m1snd_message)

// this is all the native to Java stuff
	
public class NDKCallbacks {
	static TextView Title;
	static TextView PlayTime;
	static TextView TrackNum;
	static boolean loadError = false;
	static GameListAdapter nonglobalgla = new GameListAdapter();	
	static PlayerService playerService = new PlayerService();
	static String m1error;
	static Context ctx;
	static public void setTitleView(TextView tv){
		Title = tv;
	}

	public static void SetGameName(String name){
		Title.setText(name);
	}
	
	 

	public static void addROM(String name){
		if(name!=null){
			GameList it = new GameList(name);
			Log.v("M1Android", "Adding item");
			nonglobalgla.addItem(it);
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
		Toast.makeText(ctx, NDKCallbacks.m1error, Toast.LENGTH_SHORT).show();
	}
	public static void Silence(){
		NDKBridge.next();
		playerService.setNoteText();

		// if we are to skip songs when we here silence, this is where we do it
	}
	
}
