package com.neko68k.M1;

import android.content.Context;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Created by neko on 4/18/15.
 */
public class ChannelListView extends LinearLayout{
    private TextView trackNum;
    private TextView mText;
    private TextView time;
    private CheckBox fave;

    public ChannelListView(Context context, ChannelList channelList) {
        super(context);

        this.setOrientation(HORIZONTAL);

        mText = new TextView(context);
        trackNum = new TextView(context);
        time = new TextView(context);
        fave = new CheckBox(context);

        //mText.setText(trackList.getText());
        //trackNum.setText(trackList.getTrackNum());
        //time.setText(trackList.getTime());
        //fave.setChecked(false);

        //mText.setTextSize(17);
        //time.setTextSize(17);
        //trackNum.setTextSize(17);

        //fave.setFocusable(false);
        //mText.setFocusable(false);
        //trackNum.setFocusable(false);
        //time.setFocusable(false);


        LinearLayout.LayoutParams layout =new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        LinearLayout.LayoutParams rightlayout =new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        rightlayout.weight = 0.69f;

        //addView(trackNum, layout);
        //addView(mText, rightlayout);
        //addView(time, layout);
        //addView(fave, layout);
    }

    public void setText(String words) {
        mText.setText(words);
    }

    public void setTrackNum(String trackNum2) {
        trackNum.setText(trackNum2);
    }

    public void setTime(String time2) {
        time.setText(time2);
    }

}
