package com.neko68k.M1;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.view.View;
import android.widget.ToggleButton;

import java.util.Map;

public class FragmentControl extends FragmentActivity implements
		GameListFrag.OnItemSelectedListener, GameListOptionsFrag.OnOptionsChanged, FileBrowser.FBCallback{

    public static final String ACTION_LOAD_COMPLETE = "com.neko68k.M1.action.TOGGLE_LOAD_COMPLETE";
	private static boolean filtered = false;
	private static boolean sorted = false;
	private static int sortType = 0;
	private static boolean faves = false;
    boolean mIsBound = false;
    InitM1Task task;
    Map<String, ?> preferences;
    public PlayerService playerService = new PlayerService();
    Messenger mService = null;
    private boolean isPlaying = false;
    private MainDetailFragment mdf=null;

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
                //NDKBridge.inited = savedInstanceState.getBoolean("inited");
                FragmentManager mFragmentManager = getSupportFragmentManager();
                //mFragmentManager.putFragment(inBundle, "main", (MainDetailFragment) mFragmentManager.findFragmentById(R.id.details));
                mdf = (MainDetailFragment)mFragmentManager.getFragment(savedInstanceState, "main");
			//	return;
			}

			// Create a new Fragment to be placed in the activity layoutra


        FragmentManager mFragmentManager = getSupportFragmentManager();
        //MainDetailFragment fragment = (MainDetailFragment) mFragmentManager.findFragmentById(R.id.details);
        //mdf=fragment;
        if (mdf == null) {
            MainDetailFragment detailFragment = new MainDetailFragment();
            PlayerControlFragment playerFragment = new PlayerControlFragment();
            TrackListFragment trackFragment = new TrackListFragment();
            detailFragment.setRetainInstance(true);
            trackFragment.setRetainInstance(true);
            FragmentTransaction fragmentTransaction = mFragmentManager.beginTransaction();
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.details, detailFragment).add(R.id.tracklist, trackFragment).
                    add(R.id.playercontrols, playerFragment).commit();
        }


        NDKBridge.ctx = getApplicationContext();
	}

    @Override
    public void onNewIntent (Intent intent){
        if(intent!=null) {
            String action = intent.getAction();
            if (action != null) {
                if (action.equals(ACTION_LOAD_COMPLETE)) {
                    if (!NDKBridge.loadError) {
                        mdf = (MainDetailFragment) getSupportFragmentManager().findFragmentById(R.id.details);
                        TrackListFragment tlf = (TrackListFragment) getSupportFragmentManager().findFragmentById(R.id.tracklist);
                        PlayerControlFragment pcf = (PlayerControlFragment) getSupportFragmentManager().findFragmentById(R.id.playercontrols);
                        mdf.updateDetails();
                        tlf.updateTrackList();
                        NDKBridge.playtime = 0;
                        mdf.updateTrack();

                        isPlaying=true;

                        pcf.setPlayState(isPlaying);
                    }
                }
            }
        }
    }


    @Override
    public void onSaveInstanceState(Bundle inBundle){
        super.onSaveInstanceState(inBundle);
        inBundle.putBoolean("playstate", isPlaying);
        FragmentManager mFragmentManager = getSupportFragmentManager();
        mFragmentManager.putFragment(inBundle, "main", mFragmentManager.findFragmentById(R.id.details));

    }

    @Override
    public void onRestart(){
        super.onRestart();
        isPlaying = playerService.getPlaying();
        PlayerControlFragment pcf = (PlayerControlFragment) getSupportFragmentManager().findFragmentById(R.id.playercontrols);
        pcf.setPlayState(isPlaying);
    }

   @Override
    public void onRestoreInstanceState(Bundle inBundle){
       super.onRestoreInstanceState(inBundle);
        if(inBundle!=null){
            doBindService();
        }
       PlayerControlFragment pcf = (PlayerControlFragment) getSupportFragmentManager().findFragmentById(R.id.playercontrols);
       isPlaying = inBundle.getBoolean("playstate");
       pcf.setPlayState(isPlaying);
    }



    @Override
    public void onBackPressed() {
        if (mIsBound) {

            this.startService(new Intent(PlayerService.ACTION_STOP, null, this.getApplicationContext(), PlayerService.class));
            doUnbindService();
        }

        this.finish();
        return;
    }

    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case NDKBridge.MSG_UPDATE_TIME:
                    break;
                case NDKBridge.MSG_UPDATE_TRACK:
                    /*MainDetailFragment detailFragment = new MainDetailFragment();
                    PlayerControlFragment playerFragment = new PlayerControlFragment();
                    TrackListFragment trackFragment = new TrackListFragment();
*/

                    FragmentManager mFragmentManager = getSupportFragmentManager();
                    MainDetailFragment fragment = (MainDetailFragment) mFragmentManager.findFragmentById(R.id.details);

                      /*                  if (fragment == null) {
                        //FragmentTransaction fragmentTransaction = mFragmentManager.beginTransaction();
                        getSupportFragmentManager().beginTransaction()
                                .add(R.id.details, detailFragment).add(R.id.tracklist, trackFragment).
                                add(R.id.playercontrols, playerFragment).commit();
                    }*/
                    if(mdf!=null)
                        mdf.updateTrack();
                    break;
                case NDKBridge.MSG_TOGGLE_PAUSE:
                    isPlaying = true;
                    break;
                case NDKBridge.MSG_TOGGLE_PLAY:
                    isPlaying = false;
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    final Messenger mMessenger = new Messenger(new IncomingHandler());

    // service connection stuff
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            mService = new Messenger(service);

            Message msg = Message.obtain(null,
                    NDKBridge.MSG_REGISTER_CLIENT);
            msg.replyTo = mMessenger;
            try {
                mService.send(msg);
            } catch(RemoteException e){

            }
        }

        public void onServiceDisconnected(ComponentName className) {
            mService = null;
        }
    };

    void doBindService() {
        getApplicationContext().bindService(new Intent(NDKBridge.ctx, PlayerService.class),
                mConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
    }

    void doUnbindService() {
        if (mIsBound) {
            getApplicationContext().unbindService(mConnection);
            if (mService != null) {
                try {
                    Message msg = Message.obtain(null,
                            NDKBridge.MSG_UNREGISTER_CLIENT);
                    msg.replyTo = mMessenger;
                    mService.send(msg);
                } catch (RemoteException e) {
                }
            }
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

                startService(new Intent(PlayerService.ACTION_LOAD, null, this, PlayerService.class).putExtra("gameid", NDKBridge.game.index));
                doBindService();
                NDKBridge.playtime = 0;


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
    }
	
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
