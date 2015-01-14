package com.neko68k.M1;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Binder;
import android.os.IBinder;
import android.widget.RemoteViews;
//import android.R;

// this will handle all the threading and shit
// so we can properly stop it and allow the app
// to shut down or otherwise not keep sucking
// up the battery

public class PlayerService extends Service implements MusicFocusable {
	String text;
	static Notification notification = null;
	PendingIntent contentIntent;

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

	AudioDevice ad = null;

	public class LocalBinder extends Binder {
		PlayerService getService() {
			return PlayerService.this;
		}
	}

	@Override
	public void onCreate() {
        Context ctx = getApplicationContext();
		// mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        mMediaButtonReceiverComponent = new ComponentName(ctx, MusicIntentReceiver.class);


        mAudioManager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);

        if (android.os.Build.VERSION.SDK_INT >= 8)
            mAudioFocusHelper = new AudioFocusHelper(ctx, this);
        else
            mAudioFocus = AudioFocus.Focused; // no focus feature, so we always "have" audio focus
		//play();
	}

    public void onLostAudioFocus(boolean canDuck) {
        //Toast.makeText(getApplicationContext(), "lost audio focus." + (canDuck ? "can duck" :
        //"no duck"), Toast.LENGTH_SHORT).show();
        mAudioFocus = canDuck ? AudioFocus.NoFocusCanDuck : AudioFocus.NoFocusNoDuck;
        // start/restart/pause media player with new focus settings
        if (ad.isPlaying())
            configAndStartMediaPlayer();
    }

    public void onGainedAudioFocus() {
        //Toast.makeText(getApplicationContext(), "gained audio focus.", Toast.LENGTH_SHORT).show();
        mAudioFocus = AudioFocus.Focused;
        // restart media player with new focus settings
        if (ad.isPlaying())
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
                    PendingIntent.getBroadcast(NDKBridge.ctx /*context*/,
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
                else if (action.equals(ACTION_LOAD))
                    processLoadRequest(intent.getIntExtra("gameid", -1));
            }
        }
		return START_STICKY;
	}

    private void processStopRequest(){
        stop();
    }


    private void processRestartRequest(){
        NDKBridge.restSong();
    }

    private void processLoadRequest(int gameid){
        if(ad !=null&&ad.isPlaying()){
            //stop();
            ad.PlayStop();
            NDKBridge.stop();
        }
        NDKBridge.nativeLoadROM(gameid);
        if(!NDKBridge.loadError){
            ad = new AudioDevice("deviceThread");
            play();
            updateRemoteMetadata();
            if (mRemoteControlClientCompat != null) {
                mRemoteControlClientCompat
                        .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);
            }
        }


    }

    private void processSkipRequest(){
        if (ad.isPlaying()) {
            if (!ad.isPaused()) {
                int i = NDKBridge.next();
                //trackNum.setText("Track: " + (i));

                setNoteText();
                NDKBridge.playtime = 0;
                //if (listLen)
                //    NDKBridge.getSongLen();
                //else
                    NDKBridge.songLen = NDKBridge.defLen;
                //trackList.smoothScrollToPosition(i);
                if (mRemoteControlClientCompat != null)
                updateRemoteMetadata();
            }
        }
    }


    private void processRewindRequest(){
        if (ad.isPlaying()) {
            if (!ad.isPaused()) {
                int i = NDKBridge.prevSong();
                //trackNum.setText("Track: " + (i));
                setNoteText();
                NDKBridge.playtime = 0;
                //if (listLen)
                //    NDKBridge.getSongLen();
                //else
                    NDKBridge.songLen = NDKBridge.defLen;
                //trackList.smoothScrollToPosition(i);
                if (mRemoteControlClientCompat != null)
                updateRemoteMetadata();
            }
        }
    }

    private void processPlayRequest(){
        if (ad.isPlaying()) {
            //if (ad.isPaused()) {
                tryToGetAudioFocus();
                //playButton.setText("Pause");
                //playButton.setImageResource(R.drawable.ic_action_pause);
                //NDKBridge.unPause();
                // ad.UnPause();
                unpause();
                //paused = false;
                // Tell any remote controls that our playback state is 'playing'.
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);//);
                    //updateRemoteMetadata();
                }
            //}
        }

    }

    private void processPauseRequest(){
        if (ad.isPlaying()) {
            if (!ad.isPaused()) {
                //NDKBridge.pause();
                //playButton.setText("Play");
                //playButton.setImageResource(R.drawable.ic_action_play);
                // ad.PlayPause();
                pause();
                //paused = true;
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
        if (ad.isPlaying()) {
            if (ad.isPaused()) {
                tryToGetAudioFocus();
                //playButton.setText("Pause");
                //playButton.setImageResource(R.drawable.ic_action_pause);
                //NDKBridge.unPause();
                // ad.UnPause();
                unpause();
                //paused = false;
                // Tell any remote controls that our playback state is 'playing'.
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PLAYING);
                }
            } else if (!ad.isPaused()) {
                //NDKBridge.pause();
                //playButton.setText("Play");
                //playButton.setImageResource(R.drawable.ic_action_play);
                // ad.PlayPause();
                pause();
                //paused = true;
                // Tell any remote controls that our playback state is 'paused'.
                if (mRemoteControlClientCompat != null) {
                    mRemoteControlClientCompat
                            .setPlaybackState(RemoteControlClientCompat.PLAYSTATE_PAUSED);
                }
            }
        }
        //updateRemoteMetadata();
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
        NDKBridge.stop();
		ad.PlayQuit();
		// mNM.cancelAll();
		stopForeground(true);
	}

	public void play() {


		//new Intent(this, M1AndroidFragment.class);

		setNoteText();

		ad.PlayStart();
	}

	public void setNoteText() {
        if (notification == null)
            notification = new Notification(R.drawable.icon, "",
                    System.currentTimeMillis());
		notification.flags |= Notification.FLAG_NO_CLEAR;
		RemoteViews contentView = new RemoteViews(getPackageName(),
				R.layout.custom_notification);
		contentView.setImageViewResource(R.id.image, R.drawable.ic_launcher);
		//contentView.setTextViewText(R.id.title, "M1Android");

		notification.contentView = contentView;
		int cmdNum = NDKBridge
				.getInfoInt(NDKBridge.M1_IINF_TRACKCMD, (NDKBridge.getInfoInt(
						NDKBridge.M1_IINF_CURSONG, 0) << 16 | NDKBridge
						.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0)));
		text = NDKBridge.getInfoStr(NDKBridge.M1_SINF_TRKNAME, cmdNum << 16
				| NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0));
		// text = NDKBridge.getSong(NDKBridge.getCurrentCmd());
		// text=null;
		if (text != null) {
			contentView.setTextViewText(R.id.text2, text);
			contentView
					.setTextViewText(R.id.text, NDKBridge.getInfoStr(
							NDKBridge.M1_SINF_VISNAME,
							NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0)));
		}
		if (text == null) {
			contentView.setTextViewText(R.id.text2, "No track list");
			// contentView.setTextViewText(R.id.text2,
			// NDKBridge.getGameTitle(NDKBridge.curGame).getText());
			contentView.setTextViewText(R.id.text, "FIXME");
		}

		contentIntent = PendingIntent.getActivity(this, 0, new Intent(this,
				FragmentControl.class).setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
				| Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);

		Intent notificationIntent = new Intent(this, FragmentControl.class);
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
				notificationIntent, 0);
		notification.contentIntent = contentIntent;

		startForeground(1337, notification);
	}

	//@Override
    public void onDestory() {
		ad.PlayQuit();
		//stopForeground(true);

	}

    @Override
    public boolean onUnbind(Intent intent) {
        stop();
        return false;
    }

	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}

	private final IBinder mBinder = new LocalBinder();

}
