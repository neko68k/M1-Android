package com.neko68k.M1;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import android.app.ProgressDialog;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;

public class InitM1Task extends AsyncTask<Void, Void, Void> {
	ProgressDialog dialog;

	private Context context;

	private Hashtable<Long, String> cpuHashSet = new Hashtable<Long, String>();
	private Hashtable<Long, String> sound1HashSet = new Hashtable<Long, String>();
	private Hashtable<Long, String> sound2HashSet = new Hashtable<Long, String>();
	private Hashtable<Long, String> sound3HashSet = new Hashtable<Long, String>();
	private Hashtable<Long, String> sound4HashSet = new Hashtable<Long, String>();
	
	private Hashtable<Long, String> boardHashSet = new Hashtable<Long, String>();
	private Hashtable<Long, String> mfgHashSet = new Hashtable<Long, String>();
	private HashSet<Integer> yearHashSet = new HashSet<Integer>();

	public InitM1Task(Context c) // pass the context in the constructor
	{
		context = c;
	}

	@Override
	protected void onPreExecute() {
		dialog = ProgressDialog.show(context, "",
				"Initializing. This may take a long time...", true);
		Log.v("com.neko68.M1", "Starting init...");

	}

	@Override
	protected Void doInBackground(Void... unused) {
		String outpath = null;
		Game game;
		File xml = new File(NDKBridge.basepath + "/m1/m1.xml");
		if (xml.exists() == false) {
			File extdir = Environment.getExternalStorageDirectory();
			try {
				outpath = NDKBridge.basepath + "/m1/";
				File m1dir = new File(outpath);
				m1dir.mkdirs();
			} catch (Exception e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}

			Unzip(outpath, R.raw.m1xml20111011);

			try {
				outpath = NDKBridge.basepath + "/m1/lists/";
				File m1dir = new File(outpath);
				m1dir.mkdirs();
			} catch (Exception e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
			Unzip(outpath, R.raw.lists);

			try {
				outpath = NDKBridge.basepath + "/m1/roms/";
				File m1dir = new File(outpath);
				m1dir.mkdirs();
			} catch (Exception e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
		}

		NDKBridge.initM1();
		// int numGames = NDKBridge.getMaxGames();

		int i = 0;
		NDKBridge.m1db = new GameDatabaseHelper(context);		
		// SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();

		// FIXME: for ! for debugging only
		if (!GameListOpenHelper.checkTable()) {
			game = new Game();
			CRC32 crc = new CRC32();
			int maxGames = NDKBridge.getMaxGames();
			for (i = 0; i < maxGames; i++) {
				NDKBridge.cur = i;

				game = NDKBridge.queryRom(i);
				game.romavail = NDKBridge.simpleAudit(i);
				game.index = i;
				
				
				
				crc.update(game.mfg.getBytes());
				mfgHashSet.put(crc.getValue(), game.mfg);
				crc.reset();
				
				crc.update(game.sys.getBytes());
				boardHashSet.put(crc.getValue(), game.sys);
				crc.reset();
				Integer year;
				try{
				year = new Integer(game.year);
				} catch(java.lang.NumberFormatException e){
					year = 1;
				}
				yearHashSet.add(year);
				
				game.soundhw = game.cpu;
				String soundary[] = game.cpu.split(", ");
				switch (soundary.length) {
				case 5:
					game.setSound4(soundary[4]);
					crc.update(soundary[4].getBytes());
					sound4HashSet.put(crc.getValue(), soundary[4]);
					crc.reset();
				case 4:
					game.setSound3(soundary[3]);
					crc.update(soundary[3].getBytes());
					sound3HashSet.put(crc.getValue(), soundary[3]);
					crc.reset();
				case 3:
					game.setSound2(soundary[2]);
					crc.update(soundary[2].getBytes());
					sound2HashSet.put(crc.getValue(), soundary[2]);
					crc.reset();
				case 2:
					game.setSound1(soundary[1]);
					crc.update(soundary[1].getBytes());
					sound1HashSet.put(crc.getValue(), soundary[1]);
					crc.reset();
				case 1:
					game.setCpu(soundary[0]);
					crc.update(soundary[0].getBytes());
					cpuHashSet.put(crc.getValue(), soundary[0]);
					crc.reset();
				case 0:
					break;
				}
				// if(game.romavail==1){
				GameListOpenHelper.addGame(game);
				// }
			}

			GameListOpenHelper.addCPU("!--CPU--!", Long.valueOf(0));
			GameListOpenHelper.addSound1("!--Sound Chip 1--! ", Long.valueOf(0));
			GameListOpenHelper.addSound2("!--Sound Chip 2--! ", Long.valueOf(0));
			GameListOpenHelper.addSound3("!--Sound Chip 3--! ", Long.valueOf(0));
			GameListOpenHelper.addSound4("!--Sound Chip 4--! ", Long.valueOf(0));
			
			GameListOpenHelper.addMfg("!--Manufacturer--!");
			GameListOpenHelper.addBoard("!--Board--!");
			GameListOpenHelper.addYear(0000);

			Iterator it = cpuHashSet.entrySet().iterator();
			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addCPU((String) pairs.getValue(),
						(Long) pairs.getKey());

			}
			it = sound1HashSet.entrySet().iterator();

			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addSound1((String) pairs.getValue(),
						(Long) pairs.getKey());

			}
			it = sound2HashSet.entrySet().iterator();

			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addSound2((String) pairs.getValue(),
						(Long) pairs.getKey());

			}
			it = sound3HashSet.entrySet().iterator();

			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addSound3((String) pairs.getValue(),
						(Long) pairs.getKey());

			}
			it = sound4HashSet.entrySet().iterator();

			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addSound4((String) pairs.getValue(),
						(Long) pairs.getKey());

			}
			
			it = mfgHashSet.entrySet().iterator();

			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addMfg((String) pairs.getValue());

			}
			
			it = boardHashSet.entrySet().iterator();

			while (it.hasNext()) {
				Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addBoard((String) pairs.getValue());

			}
						
			it = yearHashSet.iterator();
			while (it.hasNext()) {
				//Map.Entry pairs = (Map.Entry) it.next();
				GameListOpenHelper.addYear((Integer)it.next());

			}
			

			//db.close();
		}
		return (null);
	}

	@Override
	protected void onPostExecute(Void result) {
		// NDKBridge.globalGLA = NDKCallbacks.nonglobalgla;
		dialog.dismiss();
	}

	private void Unzip(String outpath, int res) {
		final int BUFFER = 2048;
		InputStream xmlzip = context.getResources().openRawResource(res);
		ZipInputStream zis = new ZipInputStream(new BufferedInputStream(xmlzip));
		BufferedOutputStream dest = null;
		;
		ZipEntry entry = null;
		String finalpath = outpath;

		try {
			while ((entry = zis.getNextEntry()) != null) {
				if (entry.isDirectory()) {
					finalpath = outpath + entry.getName() + "/";
					File newDir = new File(finalpath);
					newDir.mkdirs();
				} else {
					int count;
					byte[] data = new byte[BUFFER];
					FileOutputStream out = new FileOutputStream(outpath
							+ entry.getName());
					dest = new BufferedOutputStream(out, BUFFER);
					while ((count = zis.read(data, 0, BUFFER)) != -1) {
						dest.write(data, 0, count);
					}
					dest.flush();
					dest.close();
				}
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
