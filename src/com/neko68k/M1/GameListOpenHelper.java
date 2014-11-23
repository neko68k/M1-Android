package com.neko68k.M1;

import java.util.ArrayList;
import java.util.List;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

public class GameListOpenHelper {
	// main table
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
	public static final String KEY_SOUNDHW = "soundhw";
	public static final String KEY_FILTERED = "filtered";
	
	// hash tables
	public static final String KEY_YEAR_HASH = "yearhash";
	public static final String KEY_MFG_HASH = "mfghash";
	public static final String KEY_SYS_HASH = "syshash";
	public static final String KEY_CPU_HASH = "cpuhash";
	public static final String KEY_SOUND1_HASH = "sound1hash";
	public static final String KEY_SOUND2_HASH = "sound2hash";
	public static final String KEY_SOUND3_HASH = "sound3hash";
	public static final String KEY_SOUND4_HASH = "sound4hash";
	public static final String KEY_SOUND5_HASH = "sound5hash";
	
	private static final String GAMELIST_TABLE_NAME = "gamelist";

	public static final int DATABASE_VERSION = 2;
	
	private static final String GAMELIST_TABLE_CREATE = "CREATE TABLE "
			+ GAMELIST_TABLE_NAME + " (" + KEY_ID + " INTEGER PRIMARY KEY, "
			+ KEY_TITLE + " TEXT, " + KEY_YEAR_HASH + " INTEGER, " + KEY_ROMNAME
			+ " TEXT, " + KEY_MFG_HASH + " INTEGER, " + KEY_SYS_HASH + " INTEGER, " + KEY_CPU_HASH
			+ " INTEGER, " + KEY_SOUND1_HASH + " INTEGER, " + KEY_SOUND2_HASH + " INTEGER, "
			+ KEY_SOUND3_HASH + " INTEGER, " + KEY_SOUND4_HASH + " INTEGER, " +
			 KEY_SOUNDHW + " TEXT, " +
			KEY_ROMAVAIL + " INTEGER);";

	public static final String CPU_TABLE = "cputable";
	private static final String CPU_TABLE_CREATE = "CREATE TABLE " 
			+ CPU_TABLE	+ " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " 
			+ KEY_CPU_HASH + " INTEGER, "
			+ KEY_CPU + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";

	public static final String SOUND1_TABLE = "sound1";
	private static final String SOUND1_TABLE_CREATE = "CREATE TABLE "
			+ SOUND1_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_SOUND1_HASH + " INTEGER , "
			+ KEY_SOUND1 + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";

	public static final String SOUND2_TABLE = "sound2";
	private static final String SOUND2_TABLE_CREATE = "CREATE TABLE "
			+ SOUND2_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_SOUND2_HASH + " INTEGER , "
			+ KEY_SOUND2 + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";

	public static final String SOUND3_TABLE = "sound3";
	private static final String SOUND3_TABLE_CREATE = "CREATE TABLE "
			+ SOUND3_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_SOUND3_HASH + " INTEGER , "
			+ KEY_SOUND3 + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";

	public static final String SOUND4_TABLE = "sound4";
	private static final String SOUND4_TABLE_CREATE = "CREATE TABLE "
			+ SOUND4_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_SOUND4_HASH + " INTEGER , "
			+ KEY_SOUND4 + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";
	
	public static final String MFG_TABLE = "mfg";
	private static final String MFG_TABLE_CREATE = "CREATE TABLE "
			+ MFG_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_MFG_HASH + " INTEGER , "
			+ KEY_MFG + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";
	
	public static final String BOARD_TABLE = "board";
	private static final String BOARD_TABLE_CREATE = "CREATE TABLE "
			+ BOARD_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_SYS_HASH + " INTEGER , "
			+ KEY_SYS + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";
	
	public static final String YEAR_TABLE = "year";
	private static final String YEAR_TABLE_CREATE = "CREATE TABLE "
			+ YEAR_TABLE + " (" + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
			+ KEY_YEAR_HASH + " INTEGER , "
			+ KEY_YEAR + " TEXT, " 
			+ KEY_FILTERED+ " INTEGER DEFAULT 0);";
	
	public static final String dispCols = YEAR_TABLE+"."+KEY_YEAR
			+ ", " + MFG_TABLE+"."+KEY_MFG
			+ ", " + BOARD_TABLE+"."+KEY_SYS 
			+ ", " + GAMELIST_TABLE_NAME+"."+KEY_ID 
			+ ", " + GAMELIST_TABLE_NAME+"."+KEY_TITLE 
			+ ", " + GAMELIST_TABLE_NAME+"."+KEY_SOUNDHW
			+ ", " + GAMELIST_TABLE_NAME+"."+KEY_ROMNAME;
	

	
	public static final String dispTbls = YEAR_TABLE+","+MFG_TABLE+","+BOARD_TABLE+","+SOUND4_TABLE+","+
	SOUND3_TABLE+","+SOUND2_TABLE+","+SOUND1_TABLE+","+CPU_TABLE;
	
	public static int cpulist;
	public static int sound1list;
	public static int sound2list;
	public static int sound3list;
	public static int sound4list;
	public static int mfglist;
	public static int boardlist;
	public static int yearlist;
	

	public static void onCreate(SQLiteDatabase db) {
		db.execSQL(GAMELIST_TABLE_CREATE);
		db.execSQL(CPU_TABLE_CREATE);
		db.execSQL(SOUND1_TABLE_CREATE);
		db.execSQL(SOUND2_TABLE_CREATE);
		db.execSQL(SOUND3_TABLE_CREATE);
		db.execSQL(SOUND4_TABLE_CREATE);
		db.execSQL(MFG_TABLE_CREATE);
		db.execSQL(BOARD_TABLE_CREATE);
		db.execSQL(YEAR_TABLE_CREATE);
	}

	public static Boolean checkTable() {
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		Cursor cursor = null;
		cursor = db.rawQuery(
				"select count(*) FROM '"
						+ GAMELIST_TABLE_NAME + "'", null);
		cursor.moveToFirst();
		if(cursor.getInt(0)!=0){
			return(true);
		}
		else{
			return(false);
		}
	}

	public static void dropTable(SQLiteDatabase db) {
		db.execSQL("DROP TABLE IF EXISTS " + GAMELIST_TABLE_NAME);
		db.execSQL("DROP TABLE IF EXISTS " + CPU_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND1_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND2_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND3_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND4_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + MFG_TABLE_CREATE);
		db.execSQL("DROP TABLE IF EXISTS " + BOARD_TABLE_CREATE);
		db.execSQL("DROP TABLE IF EXISTS " + YEAR_TABLE_CREATE);
	}

	public static void onUpgrade(SQLiteDatabase db, int oldVersion,
			int newVersion) {
		// // Drop older table if existed
		db.execSQL("DROP TABLE IF EXISTS " + GAMELIST_TABLE_NAME);
		db.execSQL("DROP TABLE IF EXISTS " + CPU_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND1_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND2_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND3_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + SOUND4_TABLE);
		db.execSQL("DROP TABLE IF EXISTS " + MFG_TABLE_CREATE);
		db.execSQL("DROP TABLE IF EXISTS " + BOARD_TABLE_CREATE);
		db.execSQL("DROP TABLE IF EXISTS " + YEAR_TABLE_CREATE);
		// Create tables again
		onCreate(db);
	}

	public static Cursor getAllTitles(SQLiteDatabase db, boolean filtered) {
		String query;
		List<String> filters = new ArrayList<String>();
		
		query = "SELECT DISTINCT "+dispCols+" FROM "+dispTbls+" JOIN gamelist"
		+" ON cputable.cpuhash=gamelist.cpuhash AND "
		+" sound1.sound1hash=gamelist.sound1hash AND "
		+" sound2.sound2hash=gamelist.sound2hash AND "
		+" sound3.sound3hash=gamelist.sound3hash AND "
		+" sound4.sound4hash=gamelist.sound4hash AND "
		+" year.yearhash=gamelist.yearhash AND "
		+" mfg.mfghash=gamelist.mfghash AND "
		+" board.syshash=gamelist.syshash AND "
		+" gamelist.romavail = 1";
		
		if(filtered==true){
				filters.clear();
				if(cpulist!=0){
					filters.add(" cputable.filtered=1 ");
				}
				if(sound1list!=0){
					filters.add(" sound1.filtered=1 ");
				}
				if(sound2list!=0){
					filters.add(" sound2.filtered=1 ");
				}
				if(sound3list!=0){
					filters.add(" sound3.filtered=1 ");
				}
				if(sound4list!=0){
					filters.add(" sound4.filtered=1 ");
				}
				if(mfglist!=0){
					filters.add(" mfg.filtered=1 ");
				}
				if(boardlist!=0){
					filters.add(" board.filtered=1 ");
				}
				if(yearlist!=0){
					filters.add(" year.filtered=1 ");
				}
								
				if(!filters.isEmpty()){
					Object[] filterQuery = filters.toArray();
					query += " WHERE";
					for(int i = 0; i<filters.size(); i++){
						if(i>0){
							query+=" AND";
						}
						query += " " + filterQuery[i];
					}
				}					
			}
			query += " GROUP BY gamelist.title " + "ORDER BY gamelist.title ASC";
			return db.rawQuery(query, null);
	}
	
	public static Cursor getAllExtra(SQLiteDatabase db, String table, String key) {
		return (db.query(table, null, null, null, null, null, key));
	}

	public static void addExtra(String table, String key, String data, Long id){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(key+"hash", id);
		values.put(key, data);
		values.put(KEY_FILTERED,  0);
		db.insert(table, null, values);
	}
	
	public static void updateExtra(String table, String keyCol, String[] data){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_FILTERED,  1);
		for(int i = 0;i<data.length;i++){
			db.update(table, values, keyCol+"=\""+data[i]+"\"", null);
		}
	}
	
	public static void resetExtras(String table){
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_FILTERED,  0);
		db.update(table, values, null, null);
	}

	public static void addGame(Game game) {
		SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_ID, game.getIndex());
		values.put(KEY_TITLE, game.getTitle());
		values.put(KEY_YEAR_HASH, game.getIntyear());
		values.put(KEY_ROMNAME, game.getRomname());
		values.put(KEY_MFG_HASH, game.getIntmfg());
		values.put(KEY_SYS_HASH, game.getIntsys());
		values.put(KEY_CPU_HASH, game.getIntcpu());
		values.put(KEY_SOUND1_HASH, game.getIntsound1());
		values.put(KEY_SOUND2_HASH, game.getIntsound2());
		values.put(KEY_SOUND3_HASH, game.getIntsound3());
		values.put(KEY_SOUND4_HASH, game.getIntsound4());
		values.put(KEY_ROMAVAIL, game.getromavail());
		values.put(KEY_SOUNDHW, game.getSoundhw());

		// Inserting Row
		db.insert(GAMELIST_TABLE_NAME, null, values);
	}
}