package com.neko68k.M1;

import android.app.ListActivity;
import android.content.Intent;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;

public class GameListActivity extends ListActivity{
	GameListAdapter gla;
	int isRunning = 0;
	int max_games;
	
	@Override
	protected void onPause(){
		
		
		super.onPause();
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		
		
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		//Cursor cursor;
		
		/*SimpleCursorAdapter adapter = new SimpleCursorAdapter(this, 
		        R.layout.gamelist_detailed, 
		        GameListOpenHelper.getAllTitles(db), 
		        new String[] { "title", "year", "mfg", "sys", "cpu" }, 
		        new int[] { R.id.title, R.id.year, R.id.mfg, R.id.board, R.id.hardware });
		*/
		
		GameListCursorAdapter adapter = new GameListCursorAdapter(this, 
		        R.layout.gamelist_detailed, 
		        GameListOpenHelper.getAllTitles(db), 
		        new String[] { "title", "year", "mfg", "sys", "cpu" }, 
		        new int[] { R.id.title, R.id.year, R.id.mfg, R.id.board, R.id.hardware });
		
		//gla=null;
		//String fn = new String();
		//Intent intent = getIntent();
		//fn = intent.getStringExtra("com.tutorials.hellotabwidget.FN");		
        super.onCreate(savedInstanceState);        
        final ListView lv = getListView();
        lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
        	public void onItemClick(AdapterView<?> av, View v, int pos, long id) {
                onListItemClick(lv, v,pos,id);
            }
        });                
        
        max_games = NDKBridge.getMaxGames();
        
        this.setListAdapter(adapter);
        db.close();        
        
	}

	@Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
            int selectionRowID = (int)position;
            
            Game game = NDKBridge.queryRom(position);
            //LoadROMTask loadTask = new LoadROMTask(this);
            //loadTask.execute(new Integer(selectionRowID));
            
            Intent i = new Intent();
            String title = "";//NDKBridge.getGameList(selectionRowID);
        	i.putExtra("com.neko68k.M1.title", title);
        	i.putExtra("com.neko68k.M1.position", position);
        	i.putExtra("com.neko68k.M1.game", game);
        	//startActivity(i);
        	setResult(RESULT_OK, i);                	
        	finish();                       
    }
}
