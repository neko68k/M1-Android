package com.neko68k.M1;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;

public class InitM1Task extends AsyncTask<Void, Void, Void>{
	ProgressDialog dialog;
	
	private Context context;

    public InitM1Task (Context c)  //pass the context in the constructor
	{
	    context = c;
	}

	
	@Override
	protected void onPreExecute (){
		dialog = ProgressDialog.show(context, "", "Please wait, initializing...", true);	
	}
	
	@Override
	protected Void doInBackground(Void... unused){		
		NDKBridge.initM1();
		int numGames = NDKBridge.getMaxGames();
		int i =0;		
		
		//List<GameList> mItems = new ArrayList<GameList>();
		if(NDKCallbacks.nonglobalgla.isEmpty()){
			for(i = 0; i<numGames;i++){
				NDKBridge.auditROM(i);			
			}
		}
		return(null);
	}
	
	@Override
	protected void onPostExecute (Void result){
		NDKBridge.globalGLA = NDKCallbacks.nonglobalgla;		
		dialog.dismiss();	
	}
}
