package com.neko68k.M1;

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
		loadError = true;
		// pop a notification that says we failed to load the rom
	}

	public static void GenericError(){
		// pop a notification that says something bad happened
	}
	public static void Silence(){
		NDKBridge.next();
		playerService.setNoteText();

		// if we are to skip songs when we here silence, this is where we do it
	}
	
}
