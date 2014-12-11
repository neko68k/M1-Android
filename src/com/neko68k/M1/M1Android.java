package com.neko68k.M1;

import java.util.ArrayList;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDatabase;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

@TargetApi(Build.VERSION_CODES.FROYO)
public class M1Android extends Activity {
	ListView trackList;
	ImageButton nextButton;
	ImageButton prevButton;
	ImageButton restButton;
	ImageButton playButton;
	TextView trackNum;
	TextView playTime;
	TextView board;
	TextView hardware;
	TextView mfg;
	TextView song;
	TextView title;
	TextView year;
	Timer updateTimer;
	ImageView icon;

	ArrayList<TrackList> listItems = new ArrayList<TrackList>();
	TrackListAdapter adapter;

	private Handler mHandler = new Handler();
	// public PlayerService playerService = new PlayerService();
	boolean mIsBound = false;

	boolean paused = false;
	boolean playing = false;
	boolean inited = false;
	int curSong = 0;
	int playtime = 0;
	TimerTask timerTask;
	int numSongs = 0;
	int maxSongs = 0;
	Map<String, ?> preferences;
	Integer defLen; // seconds
	int skipTime = 0;
	Boolean listLen;
	Boolean normalize;
	Boolean resetNorm;
	Boolean useList;
	Integer lstLang;

	InitM1Task task;

	// private PlayerService playerService;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		TrackList item;

		// get our widget id's
		trackList = (ListView) findViewById(R.id.listView1);
		nextButton = (ImageButton) findViewById(R.id.next);
		prevButton = (ImageButton) findViewById(R.id.prev);
		restButton = (ImageButton) findViewById(R.id.rest);
		playButton = (ImageButton) findViewById(R.id.play);
		trackNum = (TextView) findViewById(R.id.trackNum);
		playTime = (TextView) findViewById(R.id.playTime);
		board = (TextView) findViewById(R.id.board);
		hardware = (TextView) findViewById(R.id.hardware);
		mfg = (TextView) findViewById(R.id.mfg);
		song = (TextView) findViewById(R.id.song);
		title = (TextView) findViewById(R.id.title);
		year = (TextView) findViewById(R.id.year);
		icon = (ImageView) findViewById(R.id.icon);
		
		//NDKBridge.setTitleView(title);

		NDKBridge.ctx = this;
		if (inited == false) {
			item = new TrackList("No game loaded");
			//listItems.add("No game loaded");
			listItems.add(item);
			adapter = new TrackListAdapter(this, listItems);
			trackList.setAdapter(adapter);
			trackList.setOnItemClickListener(mMessageClickedHandler);
			trackList.setFocusable(true);
			GetPrefs();
			if (NDKBridge.basepath != null) {
				task = new InitM1Task(NDKBridge.ctx);
				task.execute();
			}			
		}
	}

	private void GetPrefs() {
		SharedPreferences prefs = PreferenceManager
				.getDefaultSharedPreferences(this);
		preferences = prefs.getAll();

		Boolean firstRun = prefs.getBoolean("firstRun", true);
		NDKBridge.basepath = prefs.getString("basepath", null);

		if (firstRun == null || firstRun == true || NDKBridge.basepath == null) {
			AlertDialog alert = new AlertDialog.Builder(M1Android.this)
					.setTitle("First Run")
					.setMessage(
							"It looks like this is your first run. Long press on a folder to choose where to install. "
									+ "For example, select '/sdcard' and I will install to '/sdcard/m1'. "
									+ "If you already have a folder with the XML, LST and ROMs, choose its parent. For example, if you have "
									+ "'/sdcard/m1' choose '/sdcard'. "
									+ "I will not overwrite any files in it. "
									+ "\n - Structure is:"
									+ "\n   .../m1/m1.xml"
									+ "\n   .../m1/lists" + "\n   .../m1/roms")
					.setPositiveButton("OK",
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int whichButton) {
									Intent intent = new Intent().setClass(
											getApplicationContext(),
											FileBrowser.class);
									intent.putExtra("title",
											"Select install directory...");
									intent.putExtra("dirpick", true);
									startActivityForResult(intent, 65535);
									/* User clicked OK so do some stuff */
								}
							})
					.setNegativeButton("Cancel",
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int whichButton) {

									/* User clicked Cancel so do some stuff */
								}
							}).create();
			alert.show();
		}
		
		

		normalize = (Boolean) preferences.get("normPref");

		if (normalize == null)
			NDKBridge.SetOption(NDKBridge.M1_OPT_NORMALIZE, 1);
		else if (normalize)
			NDKBridge.SetOption(NDKBridge.M1_OPT_NORMALIZE, 1);
		else
			NDKBridge.SetOption(NDKBridge.M1_OPT_NORMALIZE, 0);

		resetNorm = (Boolean) preferences.get("resetNormPref");
		if (resetNorm == null)
			NDKBridge.SetOption(NDKBridge.M1_OPT_RESETNORMALIZE, 1);
		else if (resetNorm)
			NDKBridge.SetOption(NDKBridge.M1_OPT_RESETNORMALIZE, 1);
		else
			NDKBridge.SetOption(NDKBridge.M1_OPT_RESETNORMALIZE, 0);

		useList = (Boolean) preferences.get("listPref");
		if (useList == null)
			NDKBridge.SetOption(NDKBridge.M1_OPT_USELIST, 1);
		else if (useList)
			NDKBridge.SetOption(NDKBridge.M1_OPT_USELIST, 1);
		else
			NDKBridge.SetOption(NDKBridge.M1_OPT_USELIST, 0);

		String tmp = (String) preferences.get("langPref");
		if (tmp != null) {
			lstLang = Integer.valueOf(tmp);
			NDKBridge.SetOption(NDKBridge.M1_OPT_LANGUAGE, lstLang);
		}

		int time = (int) prefs.getLong("defLenHours", 0)
				+ (int) prefs.getLong("defLenMins", 300)
				+ (int) prefs.getLong("defLenSecs", 0);

		NDKBridge.defLen = Integer.valueOf(time);

		listLen = (Boolean) preferences.get("listLenPref");
		if (listLen != null) {
			if (!listLen) {
				NDKBridge.songLen = NDKBridge.defLen;
			}
		} else {
			NDKBridge.songLen = NDKBridge.defLen;
			listLen = false;
		}

		Init();
	}

	private void Init() {
		// set up the button handlers
		// NEXT

		nextButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if (playing == true) {
					if (paused == false) {
						int i = NDKBridge.next();
						trackNum.setText("Track: " + (i));

						NDKBridge.playerService.setNoteText();
						NDKBridge.playtime = 0;
						if (listLen)
							NDKBridge.getSongLen();
						else
							NDKBridge.songLen = NDKBridge.defLen;
						trackList.smoothScrollToPosition(i);
					}
				}

			}
		});
		// PREV
		prevButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if (playing == true) {
					if (paused == false) {
						int i = NDKBridge.prevSong();
						trackNum.setText("Track: " + (i));
						NDKBridge.playerService.setNoteText();
						NDKBridge.playtime = 0;
						if (listLen)
							NDKBridge.getSongLen();
						else
							NDKBridge.songLen = NDKBridge.defLen;
						trackList.smoothScrollToPosition(i);
					}
				}
			}
		});
		// STOP
		restButton.setOnClickListener(new View.OnClickListener() {
			// need to to something with this. it basically kills
			// the game now and thats kind of unfriendly
			public void onClick(View v) {
				// playing = false;
				// paused = false;
				// playButton.setText("Play");
				// ad.PlayStop();
				// NDKBridge.playerService.stop();
				// doUnbindService();
				// NDKBridge.pause();
				// NDKBridge.stop();

				NDKBridge.restSong();
				NDKBridge.playtime = 0;
			}
		});
		// PLAY
		playButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if (playing == true) {
					if (paused == true) {
						//playButton.setText("Pause");
						playButton.setImageResource(R.drawable.ic_action_pause);
						NDKBridge.unPause();
						// ad.UnPause();
						NDKBridge.playerService.unpause();
						paused = false;
					} else if (paused == false) {
						NDKBridge.pause();
						//playButton.setText("Play");
						playButton.setImageResource(R.drawable.ic_action_play);
						// ad.PlayPause();
						NDKBridge.playerService.pause();
						paused = true;
					}
				}
			}
		});
	}

	private OnItemClickListener mMessageClickedHandler = new OnItemClickListener() {
		public void onItemClick(AdapterView<?> parent, View v, int position,
				long id) {
			if (mIsBound) {
				NDKBridge.jumpSong(position);
				NDKBridge.playerService.setNoteText();
				if (listLen)
					NDKBridge.getSongLen();
				else
					NDKBridge.songLen = NDKBridge.defLen;
			}
		}
	};

	private OnItemClickListener mDoNothing = new OnItemClickListener() {
		public void onItemClick(AdapterView<?> parent, View v, int position,
				long id) {
			// NDKBridge.jumpSong(position);
		}
	};

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.menu, menu);
		return true;
	}

	private Runnable mUpdateTimeTask = new Runnable() {
		public void run() {
			// update stuff here
			runOnUiThread(new Runnable() {

				public void run() {
					Integer cursong = 0;
					if (playing == true) {
						if (paused == false) {
							int seconds = NDKBridge.getCurTime() / 60;
							if (seconds > NDKBridge.songLen) {
								trackList.smoothScrollToPosition(NDKBridge
										.next());
								if (listLen)
									NDKBridge.getSongLen();
								else
									NDKBridge.songLen = NDKBridge.defLen;
							}
							int minutes = seconds / 60;
							seconds -= minutes * 60;
							cursong = NDKBridge.getInfoInt(
									NDKBridge.M1_IINF_CURSONG, 0) + 1;// M1_IINF_CURSONG;
							trackNum.setText("Track: " + cursong);
							String tmp;
							if (NDKBridge.songLen % 60 < 10)
								tmp = ":0";
							else
								tmp = ":";
							if (seconds < 10) {

								playTime.setText("Time: " + minutes + ":0"
										+ seconds + "/" + NDKBridge.songLen
										/ 60 + tmp + NDKBridge.songLen % 60);
							} else
								playTime.setText("Time: " + minutes + ":"
										+ seconds + "/" + NDKBridge.songLen
										/ 60 + tmp + NDKBridge.songLen % 60);

							// int game = m1snd_get_info_int(M1_IINF_CURGAME,
							// 0);

							// jstring track;

							// cmdNum =
							// m1snd_get_info_int(M1_IINF_TRACKCMD,(song<<16|game));
							// __android_log_print(ANDROID_LOG_INFO,
							// "M1Android", "Cmd: %i", cmdNum);

							// String track =
							// NDKBridge.getInfoStr(NDKBridge.M1_SINF_TRKNAME,
							// cursong<<16|NDKBridge.curGame);
							int cmdNum = NDKBridge.getInfoInt(
									NDKBridge.M1_IINF_TRACKCMD, (NDKBridge
											.getInfoInt(
													NDKBridge.M1_IINF_CURSONG,
													0) << 16 | NDKBridge
											.getInfoInt(
													NDKBridge.M1_IINF_CURGAME,
													0)));
							String text = NDKBridge.getInfoStr(
									NDKBridge.M1_SINF_TRKNAME,
									cmdNum << 16
											| NDKBridge.getInfoInt(
													NDKBridge.M1_IINF_CURGAME,
													0));
							song.setText("Title: " + text);// +track);

						}
					}
				}
			});
			// retrigger task
			if (playing == true) {
				mHandler.postDelayed(mUpdateTimeTask, 100);
			}
		}
	};

	// service connection stuff
	private ServiceConnection mConnection = new ServiceConnection() {
		public void onServiceConnected(ComponentName className, IBinder service) {
			NDKBridge.playerService = ((PlayerService.LocalBinder) service)
					.getService();
		}

		public void onServiceDisconnected(ComponentName className) {
			NDKBridge.playerService = null;
		}
	};

	void doBindService() {
		bindService(new Intent(M1Android.this, PlayerService.class),
				mConnection, Context.BIND_AUTO_CREATE);
		mIsBound = true;
	}

	void doUnbindService() {
		if (mIsBound) {
			unbindService(mConnection);
			mIsBound = false;
		}
	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {

		super.onActivityResult(requestCode, resultCode, data);
		if (resultCode == RESULT_OK) {
			if (requestCode == 1) {


				if (playing == true) {

					playing = false;
					paused = true;
					NDKBridge.playerService.stop();
					doUnbindService();
					NDKBridge.playtime = 0;
				}

				NDKBridge.nativeLoadROM(NDKBridge.game.index);

				if (NDKBridge.loadError == false) {
					NDKBridge.playtime = 0;

					mHandler.post(mUpdateTimeTask);
					icon.setImageBitmap(NDKBridge.getIcon());
					title.setText(NDKBridge.game.getTitle());
					board.setText("Board: " + NDKBridge.game.sys);
					mfg.setText("Maker: " + NDKBridge.game.mfg);
					year.setText("Year: " + NDKBridge.game.year);
					hardware.setText("Hardware: " + NDKBridge.game.soundhw);
					
					playButton.setImageResource(R.drawable.ic_action_pause);

					numSongs = NDKBridge.getInfoInt(NDKBridge.M1_IINF_TRACKS,
							NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0));

					if (numSongs > 0) {

						listItems.clear();
						for (int i = 0; i < numSongs; i++) {

							int game = NDKBridge.getInfoInt(
									NDKBridge.M1_IINF_CURGAME, 0);
							int cmdNum = NDKBridge.getInfoInt(
									NDKBridge.M1_IINF_TRACKCMD,
									(i << 16 | game));
							String song = NDKBridge.getInfoStr(
									NDKBridge.M1_SINF_TRKNAME,
									(cmdNum << 16 | game));

							int songlen = NDKBridge.getInfoInt(
									NDKBridge.M1_IINF_TRKLENGTH,
									(cmdNum << 16 | game));
							if (song != null) {
								String tmp;
								if (songlen / 60 % 60 < 10)
									tmp = ":0";
								else
									tmp = ":";
								TrackList item = new TrackList();
								item.setText(song);
								item.setTime((songlen / 60 / 60) + tmp
										+ (songlen / 60 % 60));
								item.setTrackNum(i+1+".");
								listItems.add(item);
							}
						}

						trackList
								.setOnItemClickListener(mMessageClickedHandler);
						adapter.notifyDataSetChanged();
						trackList.setSelection(0);

					} else {
						listItems.clear();
						TrackList item = new TrackList("No playlist");
						listItems.add(item);
						trackList.setOnItemClickListener(mDoNothing);
						adapter.notifyDataSetChanged();
					}
					if (!mIsBound) {
						doBindService();
					} else {
						if (listLen)
							NDKBridge.getSongLen();
						else
							NDKBridge.songLen = NDKBridge.defLen;
						NDKBridge.playerService.play();

					}
					playing = true;
					paused = false;
				} else {
					listItems.clear();
					TrackList item = new TrackList("No game loaded");
					listItems.add(item);
					trackList.setOnItemClickListener(mDoNothing);
					adapter.notifyDataSetChanged();
					board.setText("");
					mfg.setText("");
					hardware.setText("");
					song.setText("");
					playTime.setText("Time:");
					trackNum.setText("Track:");

					title.setText("No game loaded");
					playButton.setImageResource(R.drawable.ic_action_play);
				}
			} else if (requestCode == 2) {
				// options returned
				// stop everything and set options
				GetPrefs();
			} else if (requestCode == 65535) {
				NDKBridge.basepath = data.getStringExtra("com.neko68k.M1.FN");
				task = new InitM1Task(NDKBridge.ctx);
				task.execute();

				SharedPreferences prefs = PreferenceManager
						.getDefaultSharedPreferences(this);
				preferences = prefs.getAll();

				SharedPreferences.Editor editor = prefs.edit();
				editor.putBoolean("firstRun", false);
				editor.putString("basepath", NDKBridge.basepath);
				editor.commit();
				Init();
			}
		}
	}

	@Override
	public void onSaveInstanceState(Bundle savedInstanceState) {
		super.onSaveInstanceState(savedInstanceState);
	}

	@Override
	public void onRestoreInstanceState(Bundle savedInstanceState) {
		super.onRestoreInstanceState(savedInstanceState);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (playing == true) {
			NDKBridge.playerService.stop();
			doUnbindService();
		}
		NDKBridge.nativeClose();
		
		this.finish();
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		Intent intent;
		InitM1Task task;
		// Handle item selection
		switch (item.getItemId()) {
		case R.id.open:

			NDKBridge.loadError = false;
			intent = new Intent(this, GameListFragment.class);
			startActivityForResult(intent, 1);
			return true;
		case R.id.options:
			intent = new Intent(this, Prefs.class);
			startActivityForResult(intent, 2);
			return true;			
		case R.id.rescan:
			SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
			GameListOpenHelper.wipeTables(db);
			task = new InitM1Task(NDKBridge.ctx);
			task.execute();
			return true;

		default:

			return super.onOptionsItemSelected(item);
		}
	}
}
