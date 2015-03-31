package com.neko68k.M1;

import android.content.Intent;
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

    boolean buttonState = false;

    public void togglePlayButton(){
        if(!buttonState){
            buttonState = true;
            playButton.setImageResource(R.drawable.ic_action_pause);
        } else {
            playButton.setImageResource(R.drawable.ic_action_play);
            buttonState = false;
        }
    }

    public void setPaused(){
        buttonState = true;
        playButton.setImageResource(R.drawable.ic_action_play);
    }

    public void restoreButtonState(){
        if(buttonState){
            playButton.setImageResource(R.drawable.ic_action_pause);
        } else {
            playButton.setImageResource(R.drawable.ic_action_play);
        }
    }

    public void setPlayState(boolean state){
        if(!state){
            buttonState = false;
            playButton.setImageResource(R.drawable.ic_action_play);
        } else {
            playButton.setImageResource(R.drawable.ic_action_pause);
            nextButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    getActivity().startService(new Intent(PlayerService.ACTION_SKIP, null, getActivity().getApplicationContext(), PlayerService.class));
                }
            });
            // PREV
            prevButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    getActivity().startService(new Intent(PlayerService.ACTION_REWIND, null, getActivity().getApplicationContext(), PlayerService.class));
                }
            });
            // REST
            restButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    getActivity().startService(new Intent(PlayerService.ACTION_RESTART, null, getActivity().getApplicationContext(), PlayerService.class));
                }
            });
            // PLAY
            playButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    getActivity().startService(new Intent(PlayerService.ACTION_TOGGLE_PLAYBACK, null, getActivity().getApplicationContext(), PlayerService.class));
                    togglePlayButton();
                }
            });
            buttonState = true;
        }
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState){
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putBoolean("state", buttonState);
    }

    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.playcontrols, container, false);
        nextButton = (ImageButton) v.findViewById(R.id.next);
        prevButton = (ImageButton) v.findViewById(R.id.prev);
        restButton = (ImageButton) v.findViewById(R.id.rest);
        playButton = (ImageButton) v.findViewById(R.id.play);

        if(savedInstanceState!=null){
            buttonState=savedInstanceState.getBoolean("state");
            //restoreButtonState();
        }



        return v;
    }


}
