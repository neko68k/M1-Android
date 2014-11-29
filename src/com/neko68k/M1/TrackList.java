package com.neko68k.M1;


public class TrackList implements Comparable<TrackList> {

	private String trackNum = "";
	private String mText = "";
	private String time = "";
	private boolean fave = false;
	private boolean mSelectable = true;

	public TrackList(String text) {
		mText = text;
	}

	public TrackList() {

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
	
	public String getTrackNum() {
		return trackNum;
	}

	public void setTrackNum(String text) {
		trackNum = text;
	}

	public String getTime() {
		return time;
	}

	public void setTime(String time) {
		this.time = time;
	}

	public boolean isFave() {
		return fave;
	}

	public void setFave(boolean fave) {
		this.fave = fave;
	}

	/** Make IconifiedText comparable by its name */
	// @Override
	public int compareTo(TrackList other) {
		if (this.mText != null)
			return this.trackNum.compareTo(other.getText());
		else
			throw new IllegalArgumentException();
	}
}
