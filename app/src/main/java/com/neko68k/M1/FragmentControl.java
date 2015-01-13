package com.neko68k.M1;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ToggleButton;

import java.util.Map;

public class FragmentControl extends FragmentActivity implements
		GameListFrag.OnItemSelectedListener, GameListOptionsFrag.OnOptionsChanged, FileBrowser.FBCallback{

	private static boolean filtered = false;
	private static boolean sorted = false;
	private static int sortType = 0;
	private static boolean faves = false;
    boolean mIsBound = false;
    InitM1Task task;
    Map<String, ?> preferences;
    public PlayerService playerService = new PlayerService();

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		// Check that the activity is using the layout version with
		// the fragment_container FrameLayout
		//if (findViewById(R.id.fragment_container) != null) {

			// However, if we're being restored from a previous state,
			// then we don't need to do anything and should return or else
			// we could end up with overlapping fragments.
			if (savedInstanceState != null) {
				return;
			}

			// Create a new Fragment to be placed in the activity layoutra
			MainDetailFragment detailFragment = new MainDetailFragment();
            PlayerControlFragment playerFragment = new PlayerControlFragment();
            TrackListFragment trackFragment = new TrackListFragment();

			// In case this activity was started with special instructions from
			// an
	// Intent, pass the Intent's extras to the fragment as arguments
			// firstFragment.setArguments(getIntent().getExtras());

			// Add the fragment to the 'fragment_container' FrameLayout
			getSupportFragmentManager().beginTransaction()
					.add(R.id.details, detailFragment).add(R.id.tracklist, trackFragment).
                    add(R.id.playercontrols, playerFragment).commit();
        NDKBridge.ctx = getApplicationContext();
            //FirstRun();
		//}

	}

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
        NDKBridge.ctx.bindService(new Intent(NDKBridge.ctx, PlayerService.class),
                mConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
    }

    void doUnbindService() {
        if (mIsBound) {
            NDKBridge.ctx.unbindService(mConnection);
            mIsBound = false;
        }
    }
	
	public static boolean isFaves(){
		return faves;
	}

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            if (requestCode == 65537) {


                //if () {

                //    playing = false;
                //    paused = true;
                    //NDKBridge.playerService.stop();
                if (mIsBound)
                {
                    startService(new Intent(PlayerService.ACTION_STOP, null, this, PlayerService.class));
                    doUnbindService();
                    NDKBridge.playtime = 0;
                }

                NDKBridge.nativeLoadROM(NDKBridge.game.index);

                if (!NDKBridge.loadError) {
                    NDKBridge.playtime = 0;

                    /*mHandler.post(mUpdateTimeTask);
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
                    }*/
                    if (!mIsBound) {
                        doBindService();
                    } //else {
                        /*if (listLen)
                            NDKBridge.getSongLen();
                        else
                            NDKBridge.songLen = NDKBridge.defLen;*/
                        //NDKBridge.playerService.play();
                        //startService(new Intent(PlayerService.ACTION_PLAY, null, this, PlayerService.class));

                    //}
                    //playing = true;
                    //paused = false;
                } else {
                    /*listItems.clear();
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
                    playButton.setImageResource(R.drawable.ic_action_play);*/
                }
            } else if (requestCode == 2) {
                // options returned
                // stop everything and set options
                //GetPrefs();
            }
        }
    }





        /*normalize = (Boolean) preferences.get("normPref");

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
        }*/




    @Override
    protected void onDestroy() {
        super.onDestroy();
        //if (playing == true) {
           // NDKBridge.playerService.stop();
        //    doUnbindService();
        //}
        //NDKBridge.nativeClose();

        //this.finish();
    }

    /*@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		Intent intent;
		InitM1Task task;
		FragmentTransaction transaction;
		
		switch (item.getItemId()) {
		case R.id.open:

			NDKBridge.loadError = false; 
			//intent = new Intent(NDKBridge.ctx, GameListActivity.class);
			//startActivityForResult(intent, 1);
			
			GameListFrag glfFragment = new GameListFrag();
			transaction = getSupportFragmentManager()
					.beginTransaction();
			transaction.replace(R.id.fragment_container, glfFragment);
			transaction.addToBackStack(null);

            /*
            NDKBridge.loadError = false;
			intent = new Intent(this, GameListFragment.class);
			startActivityForResult(intent, 1);
			return true;


			// Commit the transaction
			transaction.commit();
			return true;
		case R.id.options:
			intent = new Intent(NDKBridge.ctx, Prefs.class);
			startActivityForResult(intent, 2);
			return true;			
		case R.id.rescan:
			SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
			GameListOpenHelper.wipeTables(db);
			task = new InitM1Task(NDKBridge.ctx);
			task.execute();
			return true;
		case R.id.sortOptions:
			GameListOptionsFrag newFragment = new GameListOptionsFrag();
			// args.putInt(GameListOptionsActivity.ARG_POSITION, position);
			// newFragment.setArguments(args);

			transaction = getSupportFragmentManager()
					.beginTransaction();

			// Replace whatever is in the fragment_container view with this
			// fragment,
			// and add the transaction to the back stack so the user can
			// navigate back
			transaction.replace(R.id.fragment_container, newFragment);
			transaction.addToBackStack(null);

			// Commit the transaction
			transaction.commit();

		default:

			return super.onOptionsItemSelected(item);
		}
	}*/
	
	public void onOptionsChanged(Bundle b){
		filtered = b.getBoolean("filtered");
		sorted = b.getBoolean("sorted");
		faves = b.getBoolean("faves");
		sortType = b.getInt("sortType");		
		return;
	}
	
	public void onToggleClicked(View view) {
	    // Is the toggle on?
	    boolean on = ((ToggleButton) view).isChecked();
	    
	    if (on) {
	    	GameListOpenHelper.setAlbumFave((Integer)view.getTag(), true);
	    } else {
	    	GameListOpenHelper.setAlbumFave((Integer)view.getTag(), false);
	    }
	}
	
	public static boolean isFiltered(){
		return filtered;
	}
	public static boolean isSorted(){
		return sorted;
	}
	public static int getSortType(){
		return sortType;
	}

	public void onGameSelected(Game game) {
		// //Game game = NDKBridge.queryRom(position);

		// LoadROMTask loadTask = new LoadROMTask(this);
		// loadTask.execute(new Integer(selectionRowID));
		// game.index= position;
		
		Intent i = new Intent();
		String title = "";// NDKBridge.getGameList(selectionRowID);
		//i.putExtra("com.neko68k.M1.game", game);
		i.putExtra("com.neko68k.M1.title", title);
		i.putExtra("com.neko68k.M1.position", game.index);
		
		//i.putExtra("com.neko68k.M1.game", game);
		// startActivity(i);
		NDKBridge.game = game;
		setResult(RESULT_OK, i);
		finish();

	}
    public void selected() {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(NDKBridge.ctx);
        preferences = prefs.getAll();

        NDKBridge.basepath = (String) preferences.get("sysdir");
        NDKBridge.rompath = (String) preferences.get("romdir");
        NDKBridge.iconpath = (String) preferences.get("icondir");
        task = new InitM1Task(NDKBridge.ctx);
        task.execute();

        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean("firstRun", false);
        //editor.putString("basepath", NDKBridge.basepath);
        editor.commit();
        //Init();

    }


}
