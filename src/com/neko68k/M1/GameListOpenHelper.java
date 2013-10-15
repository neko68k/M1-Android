package com.neko68k.M1;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;


public class GameListOpenHelper extends SQLiteOpenHelper {
 

	private static final int DATABASE_VERSION = 1;
    private static final String GAMELIST_TABLE_NAME = "gamelist";
    private static final String GAMELIST_TABLE_CREATE =
                "CREATE TABLE " + GAMELIST_TABLE_NAME + " (" +
                "index" + " INTEGER PRIMARY KEY, " +
                "title" + " TEXT, " +
                "year" + " TEXT, " +
                "romname" + " TEXT, " +
                "mfg" + " TEXT, " +
                "sys" + " TEXT, " +
                "cpu" + " TEXT, " +
                "sound1" + " TEXT, " +
                "sound2" + " TEXT, " +
                "sound3" + " TEXT, " +
                "sound4" + " TEXT, " +
                "sound5" + " TEXT, " +
                "listavail" + " INTEGER);";

    GameListOpenHelper(Context context) {
        super(context, "M1Android", null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(GAMELIST_TABLE_CREATE);
    }

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + GAMELIST_TABLE_NAME);
 
        // Create tables again
        onCreate(db);
	}
	
	public void addGame(Game game) {
	    SQLiteDatabase db = this.getWritableDatabase();
	 
	    ContentValues values = new ContentValues();
	    values.put("index", game.getIndex()); 
	    values.put("title", game.getTitle()); 
	    values.put("year", game.getYear()); 
	    values.put("romname", game.getRomname()); 
	    values.put("mfg", game.getMfg()); 
	    values.put("sys", game.getSys()); 
	    values.put("cpu", game.getTitle()); 
	    values.put("sound1", game.getTitle()); 
	    values.put("sound2", game.getTitle()); 
	    values.put("sound3", game.getTitle()); 
	    values.put("sound4", game.getTitle()); 	   
	    values.put("listavail", game.getListavail()); 
	 
	    // Inserting Row
	    db.insert(GAMELIST_TABLE_NAME, null, values);
	    db.close(); // Closing database connection
	}
}