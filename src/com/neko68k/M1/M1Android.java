package com.neko68k.M1;

import java.util.ArrayList;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDatabase;
import android.media.AudioManager;
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

@SuppressLint("NewApi")
public class M1Android extends Activity implements MusicFocusable{
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
	
	// ***** Remote Control Stuff ***** //
	public static final String ACTION_TOGGLE_PLAYBACK =
            "com.neko68k.M1.action.TOGGLE_PLAYBACK";
    public static final String ACTION_PLAY = "com.neko68k.M1.action.PLAY";
    public static final String ACTION_PAUSE = "com.neko68k.M1.action.PAUSE";
    public static final String ACTION_STOP = "com.neko68k.M1.action.STOP";
    public static final String ACTION_SKIP = "com.neko68k.M1.action.SKIP";
    public static final String ACTION_REWIND = "com.neko68k.M1.action.REWIND";
    public static final String ACTION_URL = "com.neko68k.M1.action.URL";

    RemoteControlClientCompat mRemoteControlClientCompat;
    ComponentName mMediaButtonReceiverComponent;
    AudioFocusHelper mAudioFocusHelper = null;
    public static final float DUCK_VOLUME = 0.1f;
    AudioFocus mAudioFocus = AudioFocus.NoFocusNoDuck;
    AudioManager mAudioManager;
    
    enum AudioFocus {
        NoFocusNoDuck,    // we don't have audio focus, and can't duck
        NoFocusCanDuck,   // we don't have focus, but can play at a low volume ("ducking")
        Focused           // we have full audio focus
    }
    // ***** End Remote Control Stuff ***** //
    
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
		
		mMediaButtonReceiverComponent = new ComponentName(this, MusicIntentReceiver.class);
		
		
		mAudioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
		
		if (android.os.Build.VERSION.SDK_INT >= 8)
            mAudioFocusHelper = new AudioFocusHelper(getApplicationContext(), this);
        else
            mAudioFocus = AudioFocus.Focused; // no focus feature, so we always "have" audio focus

		NDKBridge.ctx = this;
		if (inited == false) {
			item = new TrackList("No game loaded");
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
				processSkipRequest();
			}
		});
		// PREV
		prevButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				processRewindRequest();
			}
		});
		// STOP
		restButton.setOnClickListener(new View.OnClickListener() {
			// need to to something with this. it basically kills
			// the game now and thats kind of unfriendly
			public void onClick(View v) {
								
				NDKBridge.restSong();
				NDKBridge.playtime = 0;
			}
		});
		// PLAY
		playButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				processTogglePlaybackRequest();
			}
		});
	}
	
	private void updateRemoteMetadata(){
		// Use the media button APIs (if available) to register ourselves for media button
        // events
		tryToGetAudioFocus();
        MediaButtonHelper.registerMediaButtonEventReceiverCompat(
                mAudioManager, mMediaButtonReceiverComponent);
        // Use the remote control APIs (if available) to set the playback state
        if (mRemoteControlClientCompat == null) {
            Intent intent = new Intent(Intent.ACTION_MEDIA_BUTTON);
            intent.setComponent(mMediaButtonReceiverComponent);
            mRemoteControlClientCompat = new RemoteControlClientCompat(
                    PendingIntent.getBroadcast(this /*context*/,
                            0 /*requestCode, ignored*/, intent /*intent*/, 0 /*flags*/));
            RemoteControlHelper.registerRemoteControlClient(mAudioManager,
                    mRemoteControlClientCompat);
        }
        mRemoteControlClientCompat.setPlaybackState(
                RemoteControlClientCompat.PLAYSTATE_PLAYING);
        mRemoteControlClientCompat.setTransportControlFlags(
                RemoteControlClientCompat.FLAG_KEY_MEDIA_PLAY |
                RemoteControlClientCompat.FLAG_KEY_MEDIA_PAUSE |
                RemoteControlClientCompat.FLAG_KEY_MEDIA_NEXT |
                RemoteControlClientCompat.FLAG_KEY_MEDIA_PREVIOUS);
        // Update the remote controls
        int cmdNum = NDKBridge
				.getInfoInt(NDKBridge.M1_IINF_TRACKCMD, (NDKBridge.getInfoInt(
						NDKBridge.M1_IINF_CURSONG, 0) << 16 | NDKBridge
						.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0)));
		String text = NDKBridge.getInfoStr(NDKBridge.M1_SINF_TRKNAME, cmdNum << 16
				| NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0));
        mRemoteControlClientCompat.editMetadata(true)
                .putString(RemoteControlClientCompat.MetadataEditorCompat.METADATA_KEY_ARTIST, NDKBridge.game.getMfg())
                .putString(RemoteControlClientCompat.MetadataEditorCompat.METADATA_KEY_ALBUM, NDKBridge.game.getTitle())
                .putString(RemoteControlClientCompat.MetadataEditorCompat.METADATA_KEY_TITLE, text)
                .putLong(RemoteControlClientCompat.MetadataEditorCompat.METADATA_KEY_DURATION,
                        NDKBridge.songLen)
                // TODO: fetch real item artwork
                //.putBitmap(
                //        RemoteControlClientCompat.MetadataEditorCompat.METADATA_KEY_ARTWORK,
                //        mDummyAlbumArt)
                .apply();
	}
	
	private void processSkipRequest(){
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
				//if (mRemoteControlClientCompat != null)
					updateRemoteMetadata();
			}
		}
	}
	
	
	private void processRewindRequest(){
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
				//if (mRemoteControlClientCompat != null)
					updateRemoteMetadata();
			}
		}
	}
	
	private void processPlayRequest(){
		if (playing == true) {
			if (paused == true) {
				tryToGetAudioFocus();
				//playButton.setText("Pause");
				playButton.setImageResource(R.drawable.ic_action_pause);
				NDKBridge.unPause();
				// ad.UnPause();
				NDKBridge.playerService.unpause();
				paused = false;
				// Tell any remote controls that our playback state is 'playing'.
		        if (mRemoteControlClientCompat != null) {
		            mRemoteControlClientCompat
		                    .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);//);
		            //updateRemoteMetadata();
		        }       
			}
		}
	
	}
	
	private void processPauseRequest(){
		if (playing == true) {
			if (paused == false) {
				NDKBridge.pause();
				//playButton.setText("Play");
				playButton.setImageResource(R.drawable.ic_action_play);
				// ad.PlayPause();
				NDKBridge.playerService.pause();
				paused = true;
				// Tell any remote controls that our playback state is 'paused'.
		        if (mRemoteControlClientCompat != null) {
		            mRemoteControlClientCompat
		                    .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PAUSED);
		            //updateRemoteMetadata();
		        }
			}
		}
	}
	
	private void processTogglePlaybackRequest(){
		if (playing == true) {
			if (paused == true) {
				tryToGetAudioFocus();
				//playButton.setText("Pause");
				playButton.setImageResource(R.drawable.ic_action_pause);
				NDKBridge.unPause();
				// ad.UnPause();
				NDKBridge.playerService.unpause();
				paused = false;
				// Tell any remote controls that our playback state is 'playing'.
		        if (mRemoteControlClientCompat != null) {
		            mRemoteControlClientCompat
		                    .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);
		        }
			} else if (paused == false) {
				NDKBridge.pause();
				//playButton.setText("Play");
				playButton.setImageResource(R.drawable.ic_action_play);
				// ad.PlayPause();
				NDKBridge.playerService.pause();
				paused = true;
				// Tell any remote controls that our playback state is 'paused'.
		        if (mRemoteControlClientCompat != null) {
		            mRemoteControlClientCompat
		                    .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PAUSED);
		        }
			}
		}
		//updateRemoteMetadata();
	}

	private OnItemClickListener mMessageClickedHandler = new OnItemClickListener() {
		public void onItemClick(AdapterView<?> parent, View v, int position,
				long id) {
			if (mIsBound) {
				NDKBridge.jumpSong(position);
				NDKBridge.playerService.setNoteText();
				if (mRemoteControlClientCompat != null)
					updateRemoteMetadata();
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
								updateRemoteMetadata();
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
							//if (mRemoteControlClientCompat != null)
								
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
	
	@Override
	protected void onNewIntent(Intent intent) {
		String action = intent.getAction();
		if(action != null){
	        if (action.equals(ACTION_TOGGLE_PLAYBACK)) processTogglePlaybackRequest();
	        else if (action.equals(ACTION_PLAY)) processPlayRequest();
	        else if (action.equals(ACTION_PAUSE)) processPauseRequest();
	        else if (action.equals(ACTION_SKIP)) processSkipRequest();
	        //else if (action.equals(ACTION_STOP)) processStopRequest();
	        else if (action.equals(ACTION_REWIND)) processRewindRequest();
		}
        
        //return START_NOT_STICKY; // Means we started the service, but don't want it to
                                 // restart in case it's killed.
	}
	
	public void onLostAudioFocus(boolean canDuck) {
        //Toast.makeText(getApplicationContext(), "lost audio focus." + (canDuck ? "can duck" :
            //"no duck"), Toast.LENGTH_SHORT).show();
        mAudioFocus = canDuck ? AudioFocus.NoFocusCanDuck : AudioFocus.NoFocusNoDuck;
        // start/restart/pause media player with new focus settings
        if (mIsBound && playing)
            configAndStartMediaPlayer();
    }
	
	public void onGainedAudioFocus() {
        //Toast.makeText(getApplicationContext(), "gained audio focus.", Toast.LENGTH_SHORT).show();
        mAudioFocus = AudioFocus.Focused;
        // restart media player with new focus settings
        if (playing)
            configAndStartMediaPlayer();
    
    }
	
	void configAndStartMediaPlayer() {
        if (mAudioFocus == AudioFocus.NoFocusNoDuck) {
            // If we don't have audio focus and can't duck, we have to pause, even if mState
            // is State.Playing. But we stay in the Playing state so that we know we have to resume
            // playback once we get the focus back.
               	if (playing == true) {
					if (paused == false) {
						NDKBridge.pause();
						playButton.setImageResource(R.drawable.ic_action_play);
						NDKBridge.playerService.pause();
						paused = true;
					}
				}
        }
        else if (mAudioFocus == AudioFocus.NoFocusCanDuck)
        	NDKBridge.playerService.setVolume(DUCK_VOLUME, DUCK_VOLUME);  // we'll be relatively quiet
        else
        	NDKBridge.playerService.setVolume(1.0f, 1.0f); // we can be loud
        if (playing == true) {
			if (paused == true) {
				playButton.setImageResource(R.drawable.ic_action_pause);
				NDKBridge.unPause();
				NDKBridge.playerService.unpause();
				paused = false;
				updateRemoteMetadata();
			} 
        }
    }

	void giveUpAudioFocus() {
        if (mAudioFocus == AudioFocus.Focused && mAudioFocusHelper != null
                                && mAudioFocusHelper.abandonFocus())
            mAudioFocus = AudioFocus.NoFocusNoDuck;
    }
	
	void tryToGetAudioFocus() {
        if (mAudioFocus != AudioFocus.Focused && mAudioFocusHelper != null
                        && mAudioFocusHelper.requestFocus())
            mAudioFocus = AudioFocus.Focused;
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
						//if (mRemoteControlClientCompat != null){							
							
							
						//}
						NDKBridge.playerService.play();
						

					}
					playing = true;
					paused = false;
					updateRemoteMetadata();
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
		giveUpAudioFocus();
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
			intent = new Intent(this, FragmentControl.class);
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
