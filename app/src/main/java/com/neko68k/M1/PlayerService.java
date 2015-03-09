package com.neko68k.M1;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;

import java.util.Map;
//import android.R;

// this will handle all the threading and shit
// so we can properly stop it and allow the app
// to shut down or otherwise not keep sucking
// up the battery

public class PlayerService extends Service implements MusicFocusable {
	String text;
	static Notification notification = null;
	PendingIntent contentIntent;
    Boolean isPlaying = false;

    // ***** Remote Control Stuff ***** //
    public static final String ACTION_TOGGLE_PLAYBACK = "com.neko68k.M1.action.TOGGLE_PLAYBACK";
    public static final String ACTION_PLAY = "com.neko68k.M1.action.PLAY";
    public static final String ACTION_PAUSE = "com.neko68k.M1.action.PAUSE";
    public static final String ACTION_STOP = "com.neko68k.M1.action.STOP";
    public static final String ACTION_SKIP = "com.neko68k.M1.action.SKIP";
    public static final String ACTION_REWIND = "com.neko68k.M1.action.REWIND";
    public static final String ACTION_URL = "com.neko68k.M1.action.URL";
    public static final String ACTION_RESTART = "com.neko68k.M1.action.RESTART";
    public static final String ACTION_LOAD = "com.neko68k.M1.action.LOAD";
    public static final String ACTION_JUMP = "com.neko68k.M1.action.JUMP";
    public static final String ACTION_UPDATE_TIME = "com.neko68k.M1.action.TIME";
    public static final String ACTION_LISTTIME = "com.neko68k.M1.action.LISTTIME";

    RemoteControlClientCompat mRemoteControlClientCompat;
    ComponentName mMediaButtonReceiverComponent;
    AudioFocusHelper mAudioFocusHelper = null;
    public static final float DUCK_VOLUME = 0.1f;
    AudioFocus mAudioFocus = AudioFocus.NoFocusNoDuck;
    AudioManager mAudioManager;
    Map<String, ?> preferences;
    SharedPreferences pref = null;

    final Messenger mMessenger = new Messenger(new IncomingHandler());
    Messenger mOutMessenger = null;
    Boolean listTime = false;
    Boolean skipping = false;

    enum AudioFocus {
        NoFocusNoDuck,    // we don't have audio focus, and can't duck
        NoFocusCanDuck,   // we don't have focus, but can play at a low volume ("ducking")
        Focused           // we have full audio focus
    }
    // ***** End Remote Control Stuff ***** //

	AudioDevice ad = null;

	public class LocalBinder extends Binder {
		PlayerService getService() {
			return PlayerService.this;
		}
	}

	@Override
	public void onCreate() {
        Context ctx = getApplicationContext();

        mMediaButtonReceiverComponent = new ComponentName(ctx, MusicIntentReceiver.class);


        mAudioManager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);

        if (android.os.Build.VERSION.SDK_INT >= 8) {
            mAudioFocusHelper = new AudioFocusHelper(ctx, this);
        }
        else
            mAudioFocus = AudioFocus.Focused; // no focus feature, so we always "have" audio focus
        //tryToGetAudioFocus();
        //giveUpAudioFocus();
        //updateRemoteMetadata();
		//play();

	}

    public void onLostAudioFocus(boolean canDuck) {
        //Toast.makeText(getApplicationContext(), "lost audio focus." + (canDuck ? "can duck" :
          //      "no duck"), Toast.LENGTH_SHORT).show();
        mAudioFocus = canDuck ? AudioFocus.NoFocusCanDuck : AudioFocus.NoFocusNoDuck;
        // start/restart/pause media player with new focus settings
        if (ad.isPlaying())
            configAndStartMediaPlayer();
    }

    public void onGainedAudioFocus() {
        //Toast.makeText(getApplicationContext(), "gained audio focus.", Toast.LENGTH_SHORT).show();
        mAudioFocus = AudioFocus.Focused;
        // restart media player with new focus settings
        //if (ad.isPlaying())
            configAndStartMediaPlayer();

    }

    void configAndStartMediaPlayer() {
        if (mAudioFocus == AudioFocus.NoFocusNoDuck) {
            // If we don't have audio focus and can't duck, we have to pause, even if mState
            // is State.Playing. But we stay in the Playing state so that we know we have to resume
            // playback once we get the focus back.
            if (ad.isPlaying()) {
                if (ad.isPaused()) {
                    //NDKBridge.pause();
                    //playButton.setImageResource(R.drawable.ic_action_play);
                    pause();
                    //paused = true;
                }
            }
        }
        else if (mAudioFocus == AudioFocus.NoFocusCanDuck)
            setVolume(DUCK_VOLUME, DUCK_VOLUME);  // we'll be relatively quiet
        else
            setVolume(1.0f, 1.0f); // we can be loud
        if (ad.isPlaying()) {
            if (ad.isPaused()) {
                //playButton.setImageResource(R.drawable.ic_action_pause);

                unpause();
                //paused = false;
                //updateRemoteMetadata();
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
                && mAudioFocusHelper.requestFocus()) {
            mAudioFocus = AudioFocus.Focused;
            //Toast.makeText(getApplicationContext(), "gained audio focus.", Toast.LENGTH_SHORT).show();
        }
    }



    private void updateRemoteMetadata(){
        // Use the media button APIs (if available) to register ourselves for media button
        // events
        //giveUpAudioFocus();
        try {
            if(mOutMessenger != null)
            mOutMessenger.send(Message.obtain(null, NDKBridge.MSG_UPDATE_TRACK));
        } catch(RemoteException e){

        }
        tryToGetAudioFocus();
        MediaButtonHelper.registerMediaButtonEventReceiverCompat(
                mAudioManager, mMediaButtonReceiverComponent);
        // Use the remote control APIs (if available) to set the playback state
        if (mRemoteControlClientCompat == null) {
            Intent intent = new Intent(Intent.ACTION_MEDIA_BUTTON);
            intent.setComponent(mMediaButtonReceiverComponent);
            mRemoteControlClientCompat = new RemoteControlClientCompat(
                    PendingIntent.getBroadcast(getApplicationContext() /*context*/,
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

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
        if(intent!=null) {
            String action = intent.getAction();
            if (action != null) {
                if (action.equals(ACTION_TOGGLE_PLAYBACK)) processTogglePlaybackRequest();
                else if (action.equals(ACTION_PLAY)) processPlayRequest();
                else if (action.equals(ACTION_PAUSE)) processPauseRequest();
                else if (action.equals(ACTION_SKIP)) processSkipRequest();
                else if (action.equals(ACTION_STOP)) processStopRequest();
                else if (action.equals(ACTION_REWIND)) processRewindRequest();
                else if (action.equals(ACTION_RESTART)) processRestartRequest();
                else if (action.equals(ACTION_JUMP)) processSongJump(intent.getIntExtra("tracknum", 1));
                else if (action.equals(ACTION_UPDATE_TIME)) processUpdateTime(intent.getLongExtra("time", 0));
                else if (action.equals(ACTION_LISTTIME)) setListTime(intent.getBooleanExtra("listtime", false));
                else if (action.equals(ACTION_LOAD))
                    processLoadRequest(intent.getIntExtra("gameid", -1));
            }
        }
		return START_STICKY;
	}

    private void processUpdateTime(long time){
        if(mOutMessenger!=null){
            Bundle bundle=new Bundle();
            bundle.putLong("time", time);
            Message msg = Message.obtain(null, NDKBridge.MSG_UPDATE_TIME);
            msg.setData(bundle);
            try {

                mOutMessenger.send(msg);
            } catch(RemoteException e){

            }
        }
        pref = PreferenceManager
                .getDefaultSharedPreferences(this);
        preferences = pref.getAll();
        /*int playtime = ((int) pref.getLong("defLenHours", 0)
                + (int) pref.getLong("defLenMins", 300)
                + (int) pref.getLong("defLenSecs", 0));*/
        Boolean listLenPref = (Boolean) preferences.get("listLenPref");
        if(listLenPref == null){
            listLenPref = true;
        }

        if(NDKBridge.GetSongLen()==-1||!listLenPref){
            NDKBridge.songLen = NDKBridge.defLen*60;
        }
        if(listLenPref&&NDKBridge.GetSongLen()!=-1){
            NDKBridge.songLen = NDKBridge.GetSongLen();
        }


            if ((!listLenPref && time >= NDKBridge.songLen) || (listLenPref && time >= NDKBridge.songLen)) {//&&NDKBridge.songLen!=-1)){
                if(skipping) {
                    return;
                }
                skipping = true;
                processSkipRequest();
            } else {
                skipping = false;
            }

    }

    public void setListTime(Boolean state){

    }

    private void processStopRequest(){
        stop();
    }

    private void processSongJump(int tracknum){
        NDKBridge.jumpSong(tracknum);
        setNoteText();
        updateRemoteMetadata();
    }

    private void processRestartRequest(){
        NDKBridge.restSong();

    }

    private void processLoadRequest(int gameid){
        if(ad !=null&&ad.isPlaying()){
            ad.PlayQuit();
            NDKBridge.stop();
        }
        NDKBridge.nativeLoadROM(gameid);
        if(!NDKBridge.loadError){
            ad = new AudioDevice("deviceThread");
            updateRemoteMetadata();
            play();
            if (mRemoteControlClientCompat != null) {
                mRemoteControlClientCompat
                        .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);
            }
            getApplicationContext().startActivity(new Intent(FragmentControl.ACTION_LOAD_COMPLETE, null, getApplicationContext(), FragmentControl.class).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            try {
                if(mOutMessenger != null)
                    mOutMessenger.send(Message.obtain(null, NDKBridge.MSG_UPDATE_TRACK));
            } catch(RemoteException e){

            }
        }


    }

    private void processSkipRequest(){

        if (ad!=null&&ad.isPlaying()) {
            if (!ad.isPaused()) {
                int i = NDKBridge.next();
                setNoteText();
                NDKBridge.playtime = 0;
                    //NDKBridge.songLen = NDKBridge.defLen;
                if (mRemoteControlClientCompat != null)
                updateRemoteMetadata();
            }
        }
    }


    private void processRewindRequest(){
        if (ad.isPlaying()) {
            if (!ad.isPaused()) {
                int i = NDKBridge.prevSong();
                setNoteText();
                NDKBridge.playtime = 0;
                    NDKBridge.songLen = NDKBridge.defLen;
                if (mRemoteControlClientCompat != null)
                updateRemoteMetadata();
            }
        }
    }

    private void processPlayRequest(){
        if (ad.isPlaying()) {
            updateRemoteMetadata();
                unpause();
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);//);

                }
        }

    }

    private void processPauseRequest(){
        if(ad == null){
            return;
        }

        if (ad.isPlaying()) {
            if (!ad.isPaused()) {
                updateRemoteMetadata();
                pause();
                try {
                    if(mOutMessenger != null)
                        mOutMessenger.send(Message.obtain(null, NDKBridge.MSG_TOGGLE_PLAY));
                } catch(RemoteException e){

                }
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PAUSED);

                }
            }
        }
    }

    private void processTogglePlaybackRequest(){
        if (ad.isPlaying()) {
            if (ad.isPaused()) {
                updateRemoteMetadata();
                unpause();
                try {
                    if(mOutMessenger != null)
                        mOutMessenger.send(Message.obtain(null, NDKBridge.MSG_TOGGLE_PAUSE));
                } catch(RemoteException e){

                }
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);
                }
                isPlaying = true;
            } else if (!ad.isPaused()) {
                updateRemoteMetadata();
                pause();
                try {
                    if(mOutMessenger != null)
                        mOutMessenger.send(Message.obtain(null, NDKBridge.MSG_TOGGLE_PLAY));
                } catch(RemoteException e){

                }
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PAUSED);
                }
                isPlaying = false;
            }
        }
    }

    public Boolean getPlaying(){
        return(isPlaying);
    }
	
	public void setVolume(float l, float r){
		ad.setVolume(l, r);
	}

	public void pause() {
        NDKBridge.pause();
		ad.PlayPause();
	}

	public void unpause() {
        NDKBridge.unPause();
		ad.UnPause();
	}

	public void stop() {
        ad.PlayQuit();
        NDKBridge.stop();
        giveUpAudioFocus();
        NDKBridge.nativeClose();
        NDKBridge.inited = false;
		stopForeground(true);
        stopSelf();
	}

	public void play() {
		setNoteText();
		ad.PlayStart();
	}

	public void setNoteText() {
        NotificationCompat.Builder mBuilder = null;
        int cmdNum = NDKBridge
                .getInfoInt(NDKBridge.M1_IINF_TRACKCMD, (NDKBridge.getInfoInt(
                        NDKBridge.M1_IINF_CURSONG, 0) << 16 | NDKBridge
                        .getInfoInt(NDKBridge.M1_IINF_CURGAME, 0)));
        text = NDKBridge.getInfoStr(NDKBridge.M1_SINF_TRKNAME, cmdNum << 16
                | NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0));
        if(text==null) {
            mBuilder =
                    new NotificationCompat.Builder(this)
                            .setSmallIcon(R.drawable.ic_launcher)
                            .setContentTitle(NDKBridge.getInfoStr(NDKBridge.M1_SINF_VISNAME, NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0)))
                            .setContentText("No Track List");
        } else {
            mBuilder =
                    new NotificationCompat.Builder(this)
                            .setSmallIcon(R.drawable.ic_launcher)
                            .setContentTitle(NDKBridge.getInfoStr(NDKBridge.M1_SINF_VISNAME, NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0)))
                            .setContentText(text);
        }
        // Creates an explicit intent for an Activity in your app
        Intent resultIntent = new Intent(this, FragmentControl.class).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);

        PendingIntent resultPendingIntent =
                PendingIntent.getActivity(this, 0, resultIntent, 0);
        mBuilder.setContentIntent(resultPendingIntent);

        Notification notification = mBuilder.build();
        startForeground(1337, notification);
	}

    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

                    case NDKBridge.MSG_REGISTER_CLIENT:
                        mOutMessenger = msg.replyTo;

                        break;
                case NDKBridge.MSG_UNREGISTER_CLIENT:
                    mOutMessenger = null;
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    @Override
    public boolean onUnbind(Intent intent) {
        //stop();
        return false;
    }

	@Override
	public IBinder onBind(Intent intent) {
		return mMessenger.getBinder();
	}

	private final IBinder mBinder = new LocalBinder();

}
