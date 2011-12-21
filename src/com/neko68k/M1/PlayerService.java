package com.neko68k.M1;


import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.widget.RemoteViews;

//import android.R;
import com.neko68k.emu.M1Android.R;

// this will handle all the threading and shit
// so we can properly stop it and allow the app
// to shut down or otherwise not keep sucking
// up the battery

public class PlayerService extends Service{
	private NotificationManager mNM;
	
	private int NOTIFICATION = 1;
	String text;
	Notification notification = null;
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
				
		if(notification == null)
			notification=new Notification(R.drawable.icon,
	                "",
	                System.currentTimeMillis());
			Intent i=new Intent(this, M1Android.class);

		setNoteText();
		ad.PlayStart();		
	}
	
	public void setNoteText(){

		notification.flags|=Notification.FLAG_NO_CLEAR;
		RemoteViews contentView = new RemoteViews(getPackageName(), R.layout.custom_notification);
		contentView.setImageViewResource(R.id.image, R.drawable.icon);
		contentView.setTextViewText(R.id.title, "M1Android");
		
		
		notification.contentView = contentView;
		
		
		text = NDKBridge.getSong(NDKBridge.getCurrentCmd());
		if(text!=null){
			contentView.setTextViewText(R.id.text, text);
			contentView.setTextViewText(R.id.text2, NDKBridge.getGameTitle(NDKBridge.curGame).getText());
		}
		if(text==null){
			contentView.setTextViewText(R.id.text, "No track list");
			contentView.setTextViewText(R.id.text2, NDKBridge.getGameTitle(NDKBridge.curGame).getText()); 
		}
		
		

		contentIntent = PendingIntent.getActivity(this, 0, 
				new Intent(this, M1Android.class).setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|
		Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
		
		Intent notificationIntent = new Intent(this, M1Android.class);
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		notification.contentIntent = contentIntent;
				
		startForeground(1337, notification);		
	}
	
	//@Override
	public void onDestory(){
		ad.PlayQuit();
		stopForeground(true);
		
	}
	
	@Override
	public IBinder onBind(Intent intent){
		return mBinder;
	}
	
	private final IBinder mBinder = new LocalBinder();
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
}
