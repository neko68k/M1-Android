package com.neko68k.M1;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.widget.RemoteViews;
//import android.R;

// this will handle all the threading and shit
// so we can properly stop it and allow the app
// to shut down or otherwise not keep sucking
// up the battery

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
		// mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		play();
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		return START_STICKY;
	}
	
	public void setVolume(float l, float r){
		ad.setVolume(l, r);
	}

	public void pause() {
		ad.PlayPause();
	}

	public void unpause() {
		ad.UnPause();
	}

	public void stop() {
		ad.PlayQuit();
		// mNM.cancelAll();
		stopForeground(true);
	}

	public void play() {

		if (notification == null)
			notification = new Notification(R.drawable.icon, "",
					System.currentTimeMillis());
		new Intent(this, M1AndroidFragment.class);

		setNoteText();
		ad.PlayStart();
	}

	public void setNoteText() {

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
				M1AndroidFragment.class).setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
				| Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);

		Intent notificationIntent = new Intent(this, M1AndroidFragment.class);
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
				notificationIntent, 0);
		notification.contentIntent = contentIntent;

		startForeground(1337, notification);
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
