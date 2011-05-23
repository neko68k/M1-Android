package com.neko68k.M1;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

// consumer
public class AudioDevice extends Thread{
	AudioTrack track;
	   //short[] buffer = new short[1024];	  
	private boolean playing = false;
	private boolean paused = false;
	   public AudioDevice(String threadName)
	   {
		   setName(threadName);
	      int minSize =AudioTrack.getMinBufferSize( 44100, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT );        
	      track = new AudioTrack( AudioManager.STREAM_MUSIC, 44100, 
	                                        AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT, 
	                                        minSize, AudioTrack.MODE_STREAM);	      
	       theProducer = new M1UpdateThread("updateThread");
	   }	   
	 
	   private M1UpdateThread theProducer; 

	   public AudioDevice( String threadName, M1UpdateThread producer ) 
	   { 
	      setName( threadName ); 
	      theProducer = producer; 
	   } 
	   
	   public void PlayStart()
	   {		   
		   track.play();
		   theProducer.PlayStart();
		   playing = true;
		   if(this.getState() == Thread.State.NEW)
		   {
			   this.start();
		   }
	   }
	   public void PlayQuit(){	   
		   track.stop();	
		   paused = true;
		   playing = false;		   		   
		   theProducer.PlayQuit();
	   }
	   public void PlayStop(){
		   track.stop();
		   paused = true;
		   playing = false;		   
		   theProducer.PlayStop();
	   }
	   public void PlayPause(){		   
		   track.pause();
		   paused = true;
		   //playing = false;
		   theProducer.PlayPause();
	   }
	   
	   public void UnPause(){		   
		   track.play();		
		   paused = false;
		   //playing = false;
		   theProducer.PlayUnPause();
	   }
	   
	   public void run() 
	   { 
	      byte buffer[]; 

	      
		   while( playing == true ){
			   while(paused == false){
			     buffer = theProducer.take_product(); 
		         track.write( buffer, 0,  ((44100/60)*2*2) ); 
		      } 	      
		   }
		   track.release();
	   } 
} 


