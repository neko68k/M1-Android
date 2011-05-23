package com.neko68k.M1;

import android.graphics.Bitmap;

public class GameList implements Comparable<GameList>{
	
	  private String mText = "";
      private Bitmap mIcon;
      private boolean mSelectable = true;
      
      public GameList(String text)
      {    	 
    	  mText = text;
      }
      
      public GameList()
      {
    	  
      }      
	public boolean isSelectable() {
        return mSelectable;
	}
	
	public void setSelectable(boolean selectable) {
	        mSelectable = selectable;
	}
	
	public String getText() {
	        return mText;
	}
	
	public void setText(String text) {
	        mText = text;
	}
	
	public void setIcon(Bitmap icon) {
	        mIcon = icon;
	}
	
	public Bitmap getIcon() {
	        return mIcon;
	}

	/** Make IconifiedText comparable by its name */
	//@Override
	public int compareTo(GameList other) {
	        if(this.mText != null)
	                return this.mText.compareTo(other.getText());
	        else
	                throw new IllegalArgumentException();
	}
}
