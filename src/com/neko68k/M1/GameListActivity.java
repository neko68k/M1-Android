package com.neko68k.M1;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

public class GameListActivity extends ListFragment{
	GameListAdapter gla;
	int isRunning = 0;
	int max_games;
	
	@Override
	public void onPause(){
		
		
		super.onPause();
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		 super.onCreate(savedInstanceState);
		
		
        
	}
	
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState){
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		//Cursor cursor;
		
		/*SimpleCursorAdapter adapter = new SimpleCursorAdapter(this, 
		        R.layout.gamelist_detailed, 
		        GameListOpenHelper.getAllTitles(db), 
		        new String[] { "title", "year", "mfg", "sys", "cpu" }, 
		        new int[] { R.id.title, R.id.year, R.id.mfg, R.id.board, R.id.hardware });
		*/
		//Context cxt = null;
		//while(cxt!=null)
			Context cxt = getActivity();
		GameListCursorAdapter adapter = new GameListCursorAdapter(cxt, 
		        R.layout.gamelist_detailed, 
		        GameListOpenHelper.getAllTitles(db), 
		        new String[] { "title", "year", "mfg", "sys", "cpu" }, 
		        new int[] { R.id.title, R.id.year, R.id.mfg, R.id.board, R.id.hardware });
		
		//gla=null;
		//String fn = new String();
		//Intent intent = getIntent();
		//fn = intent.getStringExtra("com.tutorials.hellotabwidget.FN");		
               
        final ListView lv = getListView();
        lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
        	public void onItemClick(AdapterView<?> av, View v, int pos, long id) {
                onListItemClick(lv, v,pos,id);
            }
        });                
        
        //max_games = NDKBridge.getMaxGames();
        
        this.setListAdapter(adapter);
        db.close();        
	}

	@Override
    public void onListItemClick(ListView l, View v, int position, long id) {
            Cursor cursor = (Cursor)l.getItemAtPosition(position);
            
            Game game = new Game(cursor);
            mCallback.onGameSelected(game);
                                 
    }
	
	OnItemSelectedListener mCallback;

    // Container Activity must implement this interface
    public interface OnItemSelectedListener {
        public void onGameSelected(Game game);
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        
        // This makes sure that the container activity has implemented
        // the callback interface. If not, it throws an exception
        try {
            mCallback = (OnItemSelectedListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement OnItemSelectedListener");
        }
    }
}
