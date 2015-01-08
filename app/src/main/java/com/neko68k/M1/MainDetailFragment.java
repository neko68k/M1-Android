package com.neko68k.M1;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import java.util.Timer;

/**
 * Created by neko on 1/8/15.
 */
public class MainDetailFragment extends Fragment {
    TextView trackNum;
    TextView playTime;
    TextView board;
    TextView hardware;
    TextView mfg;
    TextView song;
    TextView title;
    TextView year;
    Timer updateTimer;
    ImageView icon;

    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        
        //TrackList item;
        //View v = inflater.inflate(R.layout.main, container, false);
        //trackList = (ListView) v.findViewById(R.id.listView1);

        trackNum = (TextView) v.findViewById(R.id.trackNum);
        playTime = (TextView) v.findViewById(R.id.playTime);
        board = (TextView) v.findViewById(R.id.board);
        hardware = (TextView) v.findViewById(R.id.hardware);
        mfg = (TextView) v.findViewById(R.id.mfg);
        song = (TextView) v.findViewById(R.id.song);
        title = (TextView) v.findViewById(R.id.title);
        year = (TextView) v.findViewById(R.id.year);
        icon = (ImageView) v.findViewById(R.id.icon);

        Context ctx = getActivity();
        NDKBridge.ctx = ctx;
    }
}
