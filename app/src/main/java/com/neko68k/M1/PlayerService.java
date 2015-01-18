package com.neko68k.M1;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;

public class PlayerService extends Service {
	String text;
	Notification notification = null;
	PendingIntent contentIntent;

	AudioDevice ad = new AudioDevice("deviceThread");

	public class LocalBinder extends Binder {
		PlayerService getService() {
			return PlayerService.this;
		}
	}

	@Override
	public void onCreate() {
		play();
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		return START_STICKY;
	}

	public void pause() {
		ad.PlayPause();
	}

	public void unpause() {
		ad.UnPause();
	}

	public void stop() {
		ad.PlayQuit();
		stopForeground(true);
	}

	public void play() {

		if (notification == null)
			notification = new Notification(R.drawable.icon, "",
					System.currentTimeMillis());
		new Intent(this, M1Android.class);

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
        Intent resultIntent = new Intent(this, M1Android.class).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);

        PendingIntent resultPendingIntent =
                PendingIntent.getActivity(this, 0, resultIntent, PendingIntent.FLAG_CANCEL_CURRENT);
        mBuilder.setContentIntent(resultPendingIntent);


		startForeground(1337, mBuilder.build());
	}

	// @Override
	public void onDestory() {
		ad.PlayQuit();
		stopForeground(true);

	}

	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}

	private final IBinder mBinder = new LocalBinder();

}
