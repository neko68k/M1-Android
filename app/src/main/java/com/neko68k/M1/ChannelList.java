package com.neko68k.M1;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by neko on 4/18/15.
 */
public class ChannelList implements Comparable<TrackList>, Parcelable {

private String trackNum = "";
private String mText = "";
private String time = "";
private boolean fave = false;
private boolean mSelectable = true;

public ChannelList(String text) {
        mText = text;
        }

public ChannelList() {

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
public int compareTo(TrackList other) {
        if (this.mText != null)
        return this.trackNum.compareTo(other.getText());
        else
        throw new IllegalArgumentException();
        }

public int describeContents() {
        // TODO Auto-generated method stub
        return 0;
        }

public void writeToParcel(Parcel out, int flags) {
        out.writeString(trackNum);
        out.writeString(mText);
        out.writeString(time);
        out.writeByte((byte) (fave ? 1 : 0));
        out.writeByte((byte) (mSelectable ? 1 : 0));
        }

public ChannelList(Parcel in){
        trackNum = in.readString();
        mText = in.readString();
        time = in.readString();
        fave = in.readByte() != 0;
        mSelectable = in.readByte() != 0;
        }

public static final Parcelable.Creator<ChannelList> CREATOR = new Parcelable.Creator<ChannelList>() {
// @Override
public ChannelList createFromParcel(Parcel source) {
        return new ChannelList(source);
        }

// @Override
public ChannelList[] newArray(int size) {
        return new ChannelList[size];
        }
        };
}
