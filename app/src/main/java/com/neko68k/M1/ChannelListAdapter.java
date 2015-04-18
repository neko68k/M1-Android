package com.neko68k.M1;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.Collections;

/**
 * Created by neko on 4/18/15.
 */
public class ChannelListAdapter {
    private Context mContext;

    private ArrayList<TrackList> mItems = null;

    public ChannelListAdapter(Context context) {
        mContext = context;
    }

    public ChannelListAdapter(Context context, ArrayList<TrackList> listItems) {
        mContext = context;
        mItems = listItems;
    }


    public ChannelListAdapter() {

    }

    public void setContext(Context context) {
        mContext = context;
    }

    public boolean isEmpty() {
        return (mItems.isEmpty());
    }

    public void addItem(TrackList it) {
        mItems.add(it);
    }

    public void setListItems(ArrayList<TrackList> lit) {
        mItems = lit;
    }

    /** @return The number of items in the */
    public int getCount() {
        return mItems.size();
    }

    public Object getItem(int position) {
        return mItems.get(position);
    }

    public boolean areAllItemsSelectable() {
        return false;
    }

    public boolean isSelectable(int position) {
        try {
            return mItems.get(position).isSelectable();
        } catch (IndexOutOfBoundsException aioobe) {
            return isSelectable(position);
        }
    }

    /** Use the array index as a unique id. */
    public long getItemId(int position) {
        return position;
    }

    public void sort() {
        Collections.sort(mItems);
    }

    public String get(int i) {
        return (mItems.get(i).getText());
    }

    /**
     * @param convertView
     *            The old view to overwrite, if one is passed
     * @returns a IconifiedTextView that holds wraps around an IconifiedText
     */
    public View getView(int position, View convertView, ViewGroup parent) {
        TrackListView btv;
        if (convertView == null) {
            btv = new TrackListView(mContext, mItems.get(position));
        } else { // Reuse/Overwrite the View passed
            // We are assuming(!) that it is castable!
            btv = (TrackListView) convertView;
            btv.setText(mItems.get(position).getText());
            btv.setTrackNum(mItems.get(position).getTrackNum());
            btv.setTime(mItems.get(position).getTime());
            //btv.setFave(mItems.get(position).isFave());
        }
        return btv;
    }
}
