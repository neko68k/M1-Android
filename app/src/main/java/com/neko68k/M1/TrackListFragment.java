package com.neko68k.M1;

import android.support.v4.app.ListFragment;
import android.view.View;
import android.widget.AdapterView;

import java.util.ArrayList;

/**
 * Created by neko on 1/8/15.
 */
public class TrackListFragment extends ListFragment {
    ArrayList<TrackList> listItems = new ArrayList<TrackList>();
    TrackListAdapter adapter;

    private AdapterView.OnItemClickListener mMessageClickedHandler = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View v, int position,
                                long id) {
            //if (mIsBound) {
                //NDKBridge.jumpSong(position);
                //NDKBridge.playerService.setNoteText();
               /* if (mRemoteControlClientCompat != null)
                    updateRemoteMetadata();
                if (listLen)
                    NDKBridge.getSongLen();
                else*/
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
}
