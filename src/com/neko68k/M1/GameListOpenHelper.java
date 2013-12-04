package com.neko68k.M1;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

public class GameListOpenHelper {
	public static final String KEY_ID = "_id";
	public static final String KEY_TITLE = "title";
	public static final String KEY_YEAR = "year";
	public static final String KEY_ROMNAME = "romname";
	public static final String KEY_MFG = "mfg";
	public static final String KEY_SYS = "sys";
	public static final String KEY_CPU = "cpu";
	public static final String KEY_SOUND1 = "sound1";
	public static final String KEY_SOUND2 = "sound2";
	public static final String KEY_SOUND3 = "sound3";
	public static final String KEY_SOUND4 = "sound4";
	public static final String KEY_SOUND5 = "sound5";
	public static final String KEY_ROMAVAIL = "romavail";
	
	
	private static final int DATABASE_VERSION = 2;
    private static final String GAMELIST_TABLE_NAME = "gamelist";
    private static final String GAMELIST_TABLE_CREATE =
                "CREATE TABLE " + GAMELIST_TABLE_NAME + " (" +
                KEY_ID + " INTEGER PRIMARY KEY, " +
                KEY_TITLE + " TEXT, " +
                KEY_YEAR + " TEXT, " +
                KEY_ROMNAME + " TEXT, " +
                KEY_MFG + " TEXT, " +
                KEY_SYS + " TEXT, " +
                KEY_CPU + " TEXT, " +
                KEY_SOUND1 + " TEXT, " +
                KEY_SOUND2 + " TEXT, " +
                KEY_SOUND3 + " TEXT, " +
                KEY_SOUND4 + " TEXT, " +
                //"sound5" + " TEXT, " +
                KEY_ROMAVAIL + " INTEGER);";
    
    private static final String CPU_TABLE = "cputable";
    private static final String CPU_TABLE_CREATE = 
    		"CREATE TABLE " + CPU_TABLE + " (" +
    		KEY_ID + " INTEGER PRIMARY KEY, " +
    		KEY_CPU + " TEXT);";
    
    private static final String SOUND1_TABLE = "sound1";
    private static final String SOUND1_TABLE_CREATE = 
    		"CREATE TABLE " + SOUND1_TABLE + " (" +
    		KEY_ID + " INTEGER PRIMARY KEY, " +
    		KEY_SOUND1 + " TEXT);";
    
    private static final String SOUND2_TABLE = "sound2";
    private static final String SOUND2_TABLE_CREATE = 
    		"CREATE TABLE " + SOUND2_TABLE + " (" +
    		KEY_ID + " INTEGER PRIMARY KEY, " +
    		KEY_SOUND2 + " TEXT);";
    
    private static final String SOUND3_TABLE = "sound3";
    private static final String SOUND3_TABLE_CREATE = 
    		"CREATE TABLE " + SOUND3_TABLE + " (" +
    		KEY_ID + " INTEGER PRIMARY KEY, " +
    		KEY_SOUND3 + " TEXT);";
    
    private static final String SOUND4_TABLE = "sound4";
    private static final String SOUND4_TABLE_CREATE = 
    		"CREATE TABLE " + SOUND4_TABLE + " (" +
    		KEY_ID + " INTEGER PRIMARY KEY, " +
    		KEY_SOUND4 + " TEXT);";
    
        
    public static void onCreate(SQLiteDatabase db) {    	
    	//if(!checkTable()){
	    	db.execSQL(GAMELIST_TABLE_CREATE);
	    	db.execSQL(CPU_TABLE_CREATE);
	    	db.execSQL(SOUND1_TABLE_CREATE);
	    	db.execSQL(SOUND2_TABLE_CREATE);
	    	db.execSQL(SOUND3_TABLE_CREATE);
	    	db.execSQL(SOUND4_TABLE_CREATE);
    	//}
    }
    
    public static Boolean checkTable(){
    	SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
    	Cursor cursor =null;
    	cursor=db.rawQuery("select DISTINCT tbl_name from sqlite_master where tbl_name = '"+GAMELIST_TABLE_NAME+"'", null);
        if(cursor!=null) {
            if(cursor.getCount()>0) {
               cursor.close();
               db.close();
                return(true);
            }
               cursor.close();
               db.close();
        }
        return(false);
    }
    
    public static void dropTable(SQLiteDatabase db) {
    	db.execSQL("DROP TABLE IF EXISTS " + GAMELIST_TABLE_NAME);
    	db.execSQL("DROP TABLE IF EXISTS " + CPU_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND1_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND2_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND3_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND4_TABLE);
    	
    }

	
	public static void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + GAMELIST_TABLE_NAME);
        db.execSQL("DROP TABLE IF EXISTS " + CPU_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND1_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND2_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND3_TABLE);
    	db.execSQL("DROP TABLE IF EXISTS " + SOUND4_TABLE);
        // Create tables again
        onCreate(db);
	}
	
	public static Cursor getAllTitles(SQLiteDatabase db){
		return(db.query(GAMELIST_TABLE_NAME, null, KEY_ROMAVAIL+"=1 ", null, null, null, KEY_TITLE));
		//return(db.query(GAMELIST_TABLE_NAME, null, null, null, null, null, null));
	}
	
	public static Cursor getAllCPU(SQLiteDatabase db){
		return(db.query(CPU_TABLE, null, null, null, null, null, KEY_CPU));
	}
	
	public static Cursor getAllSound1(SQLiteDatabase db){
		return(db.query(SOUND1_TABLE, null, null, null, null, null, KEY_SOUND1));
	}
	
	public static Cursor getAllSound2(SQLiteDatabase db){
		return(db.query(SOUND2_TABLE, null, null, null, null, null, KEY_SOUND2));
	}
	
	public static Cursor getAllSound3(SQLiteDatabase db){
		return(db.query(SOUND3_TABLE, null, null, null, null, null, KEY_SOUND3));
	}
	
	public static Cursor getAllSound4(SQLiteDatabase db){
		return(db.query(SOUND4_TABLE, null, null, null, null, null, KEY_SOUND4));
	}
	
	public static void addCPU(String cpu, Long long1){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
		 
	    ContentValues values = new ContentValues();
	    values.put(KEY_ID, long1); 
	    values.put(KEY_CPU, cpu); 
	    db.insert(CPU_TABLE, null, values);
	    db.close(); // Closing database connection
	}
	
	
	public static void addSound1(String sound1, Long long1){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
		 
	    ContentValues values = new ContentValues();
	    values.put(KEY_ID, long1); 
	    values.put(KEY_SOUND1, sound1); 
	    db.insert(SOUND1_TABLE, null, values);
	    db.close(); // Closing database connection
	}
	
	public static void addSound2(String sound2, Long long1){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
		 
	    ContentValues values = new ContentValues();
	    values.put(KEY_ID, long1); 
	    values.put(KEY_SOUND2, sound2); 
	    db.insert(SOUND2_TABLE, null, values);
	    db.close(); // Closing database connection
	}
	
	public static void addSound3(String sound3, Long long1){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
		 
	    ContentValues values = new ContentValues();
	    values.put(KEY_ID, long1); 
	    values.put(KEY_SOUND3, sound3); 
	    db.insert(SOUND3_TABLE, null, values);
	    db.close(); // Closing database connection
	}
	
	public static void addSound4(String sound4, Long long1){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
		 
	    ContentValues values = new ContentValues();
	    values.put(KEY_ID, long1); 
	    values.put(KEY_SOUND4, sound4); 
	    db.insert(SOUND4_TABLE, null, values);
	    db.close(); // Closing database connection
	}
	
	public static void addGame(Game game) {
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
	 
	    ContentValues values = new ContentValues();
	    values.put(KEY_ID, game.getIndex()); 
	    values.put(KEY_TITLE, game.getTitle()); 
	    values.put(KEY_YEAR, game.getYear()); 
	    values.put(KEY_ROMNAME, game.getRomname()); 
	    values.put(KEY_MFG, game.getMfg()); 
	    values.put(KEY_SYS, game.getSys()); 
	    values.put(KEY_CPU, game.getCpu()); 
	    values.put(KEY_SOUND1, game.getTitle()); 
	    values.put(KEY_SOUND2, game.getTitle()); 
	    values.put(KEY_SOUND3, game.getTitle()); 
	    values.put(KEY_SOUND4, game.getTitle()); 	   
	    values.put(KEY_ROMAVAIL, game.getromavail()); 
	 
	    
	    // Inserting Row
	    db.insert(GAMELIST_TABLE_NAME, null, values);
	    db.close(); // Closing database connection
	}
}