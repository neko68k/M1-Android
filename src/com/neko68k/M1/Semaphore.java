package com.neko68k.M1;


public class Semaphore 
{ 
   private int     s; 
   private int     numWait; 
   private String  currentSemaphoreName; 
   private boolean doDebug; 
     
     
   public Semaphore() 
   { 
      s = 0; 
      numWait = 0; 
      currentSemaphoreName = this.toString(); 
      doDebug = false; 
   } 
  

   public Semaphore( String name, int initial, boolean debug ) 
   { 
      s = initial; 
      numWait = 0; 
      currentSemaphoreName = name; 
      doDebug = debug; 
   } 
    
    
   public void sem_wait() 
   { 
      sem_wait( Thread.currentThread().getName() ); 
   } 
    
    
   public synchronized void sem_wait( String currentThreadName ) 
   { 
      boolean retry; 
       
      /* 
      ** Check to see if semaphore was signled while no thread was waiting. 
      */ 
      if( s > 0 ) 
      { 
        s = s - 1;   // Match up one signal with one wait 
        return; 
      } 
       
      numWait = numWait + 1;     // Mark the we are going to wait. 
      do 
      { 
         retry = false; 
         try 
         { 
            if( doDebug ) 
      System.out.println( currentThreadName + " Waiting for " + currentSemaphoreName ); 
                
            this.wait();   // Do a Java wait() mutex. 
             
            if( doDebug ) 
     System.out.println( currentThreadName + " Received " + currentSemaphoreName );             
                
         } catch( InterruptedException e ) { retry = true; }          
      } while( retry ); 
   } 
  

   public void sem_signal() 
   { 
      sem_signal( Thread.currentThread().getName() ); 
   } 
    
    
   public synchronized void sem_signal( String currentThreadName ) 
   { 
      boolean retry; 

      /* 
      ** Check to see if a thread is waiting for a signal. 
      ** If not, then remember that a signal was generated without 
      ** a wait. 
      */ 
      if( numWait == 0 ) 
      { 
        s = s + 1; 
        return; 
      } 
       
      numWait = numWait - 1;   // We are going to wake up one waiting thread. 
      do 
      { 
         retry = false; 
         try 
         {   
            if( doDebug ) 
   System.out.println( currentThreadName + " Notify for " + currentSemaphoreName ); 
                
            this.notify(); 
         } 
         catch( Exception e ) { retry = true; }          
      } while( retry ); 
   }    
} 
