package com.neko68k.M1;



import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;

import com.neko68k.emu.M1Android.R;

public class M1Android extends Activity {	
	ListView trackList;
	Button nextButton;
	Button prevButton;
	Button stopButton;
	Button playButton;	
	TextView trackNum;
	TextView playTime;
	TextView board;
	TextView hardware;
	TextView mfg;
	TextView song;
	TextView title;
	Timer updateTimer;   
	
	ArrayList<String> listItems=new ArrayList<String>();
	ArrayAdapter<String> adapter;
	
	private Handler mHandler = new Handler();
//	public PlayerService playerService = new PlayerService();
	boolean mIsBound = false;
		
	boolean paused = false;
	boolean playing = false;
	boolean inited = false;
	int curSong = 0;
	int playtime = 0;
	TimerTask timerTask;
	int numSongs = 0;
	int maxSongs = 0;
	
	InitM1Task task;
	
	//private PlayerService playerService;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        // get our widget id's
        trackList = (ListView)findViewById(R.id.listView1);
        nextButton = (Button)findViewById(R.id.next);
        prevButton = (Button)findViewById(R.id.prev);
        stopButton = (Button)findViewById(R.id.stop);
        playButton = (Button)findViewById(R.id.play);
        trackNum = (TextView)findViewById(R.id.trackNum);
        playTime = (TextView)findViewById(R.id.playTime);
        board = (TextView)findViewById(R.id.board);
        hardware = (TextView)findViewById(R.id.hardware);
        mfg = (TextView)findViewById(R.id.mfg);
        song = (TextView)findViewById(R.id.song);
        title = (TextView)findViewById(R.id.title);
        NDKCallbacks.setTitleView(title);                
        
        // set up the button handlers
        // NEXT
        nextButton.setOnClickListener(new View.OnClickListener() 
        {
            public void onClick(View v) {
            	if(playing==true){
	            	if(paused==false){
	            		trackNum.setText("Command: "+(NDKBridge.next()));	
	            		NDKCallbacks.playerService.setNoteText();
	            		NDKBridge.playtime = 0;
	            	}
            	}
            }
        });
        // PREV
        prevButton.setOnClickListener(new View.OnClickListener() 
        {
            public void onClick(View v) {
            	if(playing==true){
	            	if(paused==false){
	            		trackNum.setText("Command: "+(NDKBridge.prevSong()));
	            		NDKCallbacks.playerService.setNoteText();
	            		NDKBridge.playtime = 0;
	            	}
            	}
            }
        });
        // STOP
        stopButton.setOnClickListener(new View.OnClickListener() 
        {
        	// need to to something with this. it basically kills
        	// the game now and thats kind of unfriendly
            public void onClick(View v) {  
            	playing = false;
            	paused = false;
            	playButton.setText("Play");
            	//ad.PlayStop();
            	NDKCallbacks.playerService.stop();
            	doUnbindService();
            	//NDKBridge.pause();
            	//NDKBridge.stop();
            	NDKBridge.playtime = 0;
            }
        });
        // PLAY
        playButton.setOnClickListener(new View.OnClickListener() 
        {        	
            public void onClick(View v) {
            	if(playing==true)
            	{
            		if(paused==true)
	                {
	                	playButton.setText("Pause");
	                	NDKBridge.unPause();
	                	//ad.UnPause();
	                	NDKCallbacks.playerService.unpause();
	                	paused=false;	                	
	                }
            		else if(paused==false)
	                {
	                	NDKBridge.pause();
	                	playButton.setText("Play");
	                	//ad.PlayPause();
	                	NDKCallbacks.playerService.pause();
	                	paused=true;
	                }
            	}
            }
        });
        
        if(inited==false){
	        listItems.add("No game loaded");
	        adapter=new ArrayAdapter<String>(this,
				    android.R.layout.simple_list_item_1,
				    listItems);
	        trackList.setAdapter(adapter);
	        trackList.setOnItemClickListener(mMessageClickedHandler);
	        task = new InitM1Task(this);
	        task.execute();
	        inited = true;
        }
        
        
    }
   
    private OnItemClickListener mMessageClickedHandler = new OnItemClickListener() {
        public void onItemClick(AdapterView parent, View v, int position, long id)
        {
            NDKBridge.jumpSong(position);
            NDKCallbacks.playerService.setNoteText();
        }
    };
    
    private OnItemClickListener mDoNothing= new OnItemClickListener() {
        public void onItemClick(AdapterView parent, View v, int position, long id)
        {
            //NDKBridge.jumpSong(position);
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
    		  		   if(playing==true){
    		  			   if(paused==false){
    		  				   int seconds=NDKBridge.getCurTime()/60;
    		  				   if(seconds>300){
    		  					   NDKBridge.nextSong();
    		  				   }
    		  				   int minutes=seconds/60;
    		  				   seconds -= minutes*60;
    		  				   trackNum.setText("Command: "+(NDKBridge.getCurrentCmd()+1));
    		  				   playTime.setText("Time: "+minutes+":"+seconds);
    		  				   song.setText("Song: "+NDKBridge.getSong(NDKBridge.getCurrentCmd()));
    		  				   
    		  			   }
    		  		   }
    		   	   }    		  	   
    		   	});
    		   // retrigger task
    		   if(playing==true){
    			   mHandler.postDelayed(mUpdateTimeTask, 100);
    		   }
    	   }
    };    
    
    
    // service connection stuff
    private ServiceConnection mConnection = new ServiceConnection(){
    	public void onServiceConnected(ComponentName className, IBinder service){
    		NDKCallbacks.playerService = ((PlayerService.LocalBinder)service).getService();
    	}
    	
    	public void onServiceDisconnected(ComponentName className){
    		NDKCallbacks.playerService = null;
    	}
    };
    
    void doBindService(){
    	bindService(new Intent(M1Android.this, PlayerService.class), mConnection, Context.BIND_AUTO_CREATE);
    	mIsBound = true;
    }
    
    void doUnbindService(){
    	if(mIsBound){
    		unbindService(mConnection);
    		mIsBound = false;
    	}
    }
    
    
    protected void onActivityResult (int requestCode, int resultCode, Intent data){
    	 			
    	super.onActivityResult(requestCode, resultCode, data);
    	if(requestCode == 1 && resultCode == RESULT_OK){
    		
    		//playerService.//.startService(new Intent(this, PlayerService.class));
    		
    		
    		NDKBridge.playtime = 0;
    		mHandler.post(mUpdateTimeTask);
    		board.setText("Board: "+NDKBridge.board);
			mfg.setText("Maker: "+NDKBridge.mfg);
			hardware.setText("Hardware: "+NDKBridge.hdw);
			
    		playButton.setText("Pause");
    		numSongs = NDKBridge.getNumSongs(NDKBridge.curGame);
    		if(numSongs>0){
    			
    			listItems.clear();
    			for(int i = 0; i<numSongs;i++){
    				String song = NDKBridge.getSong(i);
    				if(song!=null){
    					listItems.add((i+1)+". "+song);
    				}
    			}     
    			
    			
    			trackList.setOnItemClickListener(mMessageClickedHandler);
    			adapter.notifyDataSetChanged();

    			
    		}
    		else{
    			listItems.clear();
    			listItems.add("No playlist");
    			trackList.setOnItemClickListener(mDoNothing);
    			adapter.notifyDataSetChanged();
    		}
    		if(!mIsBound){
    			doBindService();
    		}
    		else{
    			NDKCallbacks.playerService.play();    			
    		}
    		playing=true;
    		paused=false;
    	}
    }
    
    
    
    @Override 
    protected void onDestroy(){
    	super.onDestroy();
    	if(playing==true){
    		NDKCallbacks.playerService.stop();
    		doUnbindService();
    		//ad.PlayQuit();          		
    	}    	
    	playing = false;
    	NDKBridge.nativeClose();    	
		this.finish();
    }      
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
        case R.id.open:
        	if(playing==true){
        		
        		//ad.PlayStop();
        		//doUnbindService();
        		playing = false;
        		paused = true;
        		NDKCallbacks.playerService.stop();
        		doUnbindService();
    			NDKBridge.playtime = 0;
        	}
        	Intent intent = new Intent(this, GameListActivity.class);
        	startActivityForResult(intent, 1);        	
            return true;
        case R.id.options:

            return true;

        default:
               	        	
            return super.onOptionsItemSelected(item);
        }
    }
}