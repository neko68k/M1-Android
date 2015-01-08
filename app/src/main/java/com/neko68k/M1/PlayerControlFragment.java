package com.neko68k.M1;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

/**
 * Created by neko on 1/8/15.
 */
public class PlayerControlFragment extends Fragment {
    ImageButton nextButton;
    ImageButton prevButton;
    ImageButton restButton;
    ImageButton playButton;

    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        nextButton = (ImageButton) v.findViewById(R.id.next);
        prevButton = (ImageButton) v.findViewById(R.id.prev);
        restButton = (ImageButton) v.findViewById(R.id.rest);
        playButton = (ImageButton) v.findViewById(R.id.play);

        nextButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                processSkipRequest();
            }
        });
        // PREV
        prevButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                processRewindRequest();
            }
        });
        // STOP
        restButton.setOnClickListener(new View.OnClickListener() {
            // need to to something with this. it basically kills
            // the game now and thats kind of unfriendly
            public void onClick(View v) {

                NDKBridge.restSong();
                NDKBridge.playtime = 0;
            }
        });
        // PLAY
        playButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                processTogglePlaybackRequest();
            }
        });
    }
}
