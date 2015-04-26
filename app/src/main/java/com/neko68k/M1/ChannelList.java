package com.neko68k.M1;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by neko on 4/18/15.
 */
public class ChannelList implements Comparable<ChannelList>, Parcelable {

private int channelNum = 0;
private String channelName = "";
private int volume = 0;
private int panPot = 0;
private boolean mSelectable = true;

public ChannelList(int num, String text) {
        channelName = text;
        channelNum = num;
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
        return channelName;
        }

public void setText(String text) {
        channelName = text;
        }

public void setVolume(int v) {
        volume = v;
        }

public void setPanPot(int p){
        panPot = p;
}

public int compareTo(ChannelList other) {
        if (this.channelName != null)
        return this.channelName.compareTo(other.getText());
        else
        throw new IllegalArgumentException();
        }

public int describeContents() {
        // TODO Auto-generated method stub
        return 0;
        }

public void writeToParcel(Parcel out, int flags) {
        out.writeInt(channelNum);
        out.writeString(channelName);
        out.writeInt(volume);
        out.writeInt(panPot);
        out.writeByte((byte) (mSelectable ? 1 : 0));
        }

public ChannelList(Parcel in){
        channelNum = in.readInt();
        channelName = in.readString();
        volume = in.readInt();
        panPot = in.readInt();
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
