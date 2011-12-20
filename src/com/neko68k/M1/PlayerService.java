package com.neko68k.M1;

import android.R;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

// this will handle all the threading and shit
// so we can properly stop it and allow the app
// to shut down or otherwise not keep sucking
// up the battery

public class PlayerService extends Service{
	private NotificationManager mNM;
	
	private int NOTIFICATION = 1;
	String text;
	Notification notification;
	PendingIntent contentIntent;
	
	AudioDevice ad = new AudioDevice("deviceThread");	

	public class LocalBinder extends Binder{
		PlayerService getService(){
			return PlayerService.this;
		}
	}
	
	@Override
	public void onCreate(){
		//mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		play();
	}
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId){
		return START_STICKY;
	}
	
	public void pause(){
		ad.PlayPause();
	}
	public void unpause(){
		ad.UnPause();
	}
	public void stop(){
		ad.PlayQuit();
		//mNM.cancelAll();
		stopForeground(true);
	}
	public void play(){
		notification=new Notification(R.drawable.ic_media_play,
                "",
                System.currentTimeMillis());
		Intent i=new Intent(this, M1Android.class);
		
		i.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|
		Intent.FLAG_ACTIVITY_SINGLE_TOP);
		
		PendingIntent pi=PendingIntent.getActivity(this, 0,
		                        i, 0);
		
		//notification.setLatestEventInfo(this, "M1 Android",
		    //"Now Playing: \"Ummmm, Nothing\"",
		    //pi);
		setNoteText();
		notification.flags|=Notification.FLAG_NO_CLEAR;
		
		startForeground(1337, notification);
		ad.PlayStart();
		//showNotification();
	}
	
	public void setNoteText(){

		text = NDKBridge.getGameTitle(NDKBridge.curGame).getText()+"\n"+NDKBridge.getSong(NDKBridge.getCurrentCmd());
		if(text==null){
			text = NDKBridge.getGameTitle(NDKBridge.curGame).getText();
		}

		contentIntent = PendingIntent.getActivity(this, 0, 
				new Intent(this, M1Android.class).setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|
		Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
		
		notification.setLatestEventInfo(this, "M1 Android", text, contentIntent);
		startForeground(1337, notification);
		//notification.flags = Notification.FLAG_NO_CLEAR|Notification.DEFAULT_SOUND|Notification.DEFAULT_VIBRATE|Notification.DEFAULT_LIGHTS;
		//mNM.notify(NOTIFICATION, notification);		
	}
	
	//@Override
	public void onDestory(){
		mNM.cancelAll();//(NOTIFICATION);
		ad.PlayQuit();
	}
	
	@Override
	public IBinder onBind(Intent intent){
		return mBinder;
	}
	
	private final IBinder mBinder = new LocalBinder();
	
	private void showNotification(){
		
		 //notification = new Notification(R.drawable.ic_media_play, text, System.currentTimeMillis());
		 setNoteText();
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
}
