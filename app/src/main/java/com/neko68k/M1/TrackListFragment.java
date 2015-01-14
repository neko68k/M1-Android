package com.neko68k.M1;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

import java.util.ArrayList;

/**
 * Created by neko on 1/8/15.
 */
public class TrackListFragment extends ListFragment {
    ArrayList<TrackList> listItems = new ArrayList<TrackList>();
    TrackListAdapter adapter;
    ListView trackList;

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {

        trackList = this.getListView();
        trackList.setOnItemClickListener(mDoNothing);

        listItems.add(new TrackList("No game loaded"));
        adapter = new TrackListAdapter(getActivity(), listItems);

        this.setListAdapter(adapter);

        //updateTrackList();
        /*if(savedInstanceState!=null){
            boolean inited = savedInstanceState.getBoolean("inited");
            if(inited) {
                listItems = savedInstanceState.getParcelableArrayList("gamelist");
                trackList.setOnItemClickListener(mMessageClickedHandler);
                adapter.notifyDataSetChanged();
                trackList.setSelection(0);
            }
        }*/
    }

    /*@Override
    public void onSaveInstanceState (Bundle outState){
        //outState.putBoolean("inited", NDKBridge.inited);
        //outState.putParcelableArrayList("gamelist", listItems);
    }*/

    private AdapterView.OnItemClickListener mMessageClickedHandler = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View v, int position,
                                long id) {
            getActivity().startService(new Intent(PlayerService.ACTION_JUMP, null, getActivity().getApplicationContext(),
                    PlayerService.class).putExtra("tracknum", position));
            //if (mIsBound) {
            //NDKBridge.jumpSong(position);
            //NDKBridge.playerService.setNoteText();
              //  if (mRemoteControlClientCompat != null)
                    //updateRemoteMetadata();
                //if (listLen)
                //    NDKBridge.getSongLen();
                //else
            //NDKBridge.songLen = NDKBridge.defLen;
            // }
        }
    };

    private AdapterView.OnItemClickListener mDoNothing = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View v, int position,
                                long id) {
            // do nothing
        }
    };

    public void updateTrackList() {
        int numSongs = NDKBridge.getInfoInt(NDKBridge.M1_IINF_TRACKS,
                NDKBridge.getInfoInt(NDKBridge.M1_IINF_CURGAME, 0));
        listItems.clear();
        adapter.notifyDataSetChanged();
        if (NDKBridge.game!=null&&numSongs > 0) {

            for (int i = 0; i < numSongs; i++) {

                int game = NDKBridge.getInfoInt(
                        NDKBridge.M1_IINF_CURGAME, 0);
                int cmdNum = NDKBridge.getInfoInt(
                        NDKBridge.M1_IINF_TRACKCMD,
                        (i << 16 | game));
                String song = NDKBridge.getInfoStr(
                        NDKBridge.M1_SINF_TRKNAME,
                        (cmdNum << 16 | game));

                int songlen = NDKBridge.getInfoInt(
                        NDKBridge.M1_IINF_TRKLENGTH,
                        (cmdNum << 16 | game));
                if (song != null) {
                    String tmp;
                    if (songlen / 60 % 60 < 10)
                        tmp = ":0";
                    else
                        tmp = ":";
                    TrackList item = new TrackList();
                    item.setText(song);
                    item.setTime((songlen / 60 / 60) + tmp
                            + (songlen / 60 % 60));
                    item.setTrackNum(i + 1 + ".");
                    listItems.add(item);
                }
            }

            trackList.setOnItemClickListener(mMessageClickedHandler);
            adapter.notifyDataSetChanged();
            trackList.setSelection(0);
        } else if(NDKBridge.game!=null&&numSongs <= 0){
            listItems.clear();
            TrackList item = new TrackList("No playlist");
            listItems.add(item);
            trackList.setOnItemClickListener(mDoNothing);
            adapter.notifyDataSetChanged();
        } else if(NDKBridge.game ==null) {
            listItems.clear();
            trackList.setOnItemClickListener(mDoNothing);
            listItems.add(new TrackList("No game loaded"));
            adapter = new TrackListAdapter(getActivity(), listItems);
            adapter.notifyDataSetChanged();
        }
    }
}
