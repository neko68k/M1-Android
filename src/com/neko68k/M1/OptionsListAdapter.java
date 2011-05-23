package com.neko68k.M1;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

public class OptionsListAdapter extends BaseAdapter{
	/** Remember our context so we can use it when constructing views. */
    private Context mContext;

    private List<OptionsList> mItems = new ArrayList<OptionsList>();

    public OptionsListAdapter(Context context) {
            mContext = context;
    }

    public void addItem(OptionsList it) { mItems.add(it); }

    public void setListItems(List<OptionsList> lit) { mItems = lit; }

    /** @return The number of items in the */
    public int getCount() { return mItems.size(); }

    public Object getItem(int position) { return mItems.get(position); }

    public boolean areAllItemsSelectable() { return false; }

    public boolean isSelectable(int position) {
            try{
                    return mItems.get(position).isSelectable();
            }catch (IndexOutOfBoundsException aioobe){
                    return isSelectable(position);
            }
    }

    /** Use the array index as a unique id. */
    public long getItemId(int position) {
            return position;
    }
    
    public void sort(){
    	Collections.sort(mItems);
    }
    
    public String get(int i){
    	return(mItems.get(i).getText());
    }

    /** @param convertView The old view to overwrite, if one is passed
     * @returns a IconifiedTextView that holds wraps around an IconifiedText */
    public View getView(int position, View convertView, ViewGroup parent) {
            OptionsListView btv;
            if (convertView == null) {
                    btv = new OptionsListView(mContext, mItems.get(position));
            } else { // Reuse/Overwrite the View passed
                    // We are assuming(!) that it is castable!
                    btv = (OptionsListView) convertView;
                    btv.setText(mItems.get(position).getText());
                    //btv.setIcon(mItems.get(position).getIcon());
            }
            return btv;
    }
}
