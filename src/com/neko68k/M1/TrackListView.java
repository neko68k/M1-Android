package com.neko68k.M1;

import android.content.Context;
import android.view.Gravity;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TrackListView extends LinearLayout {
	private TextView trackNum;
	private TextView mText;
	private TextView time;
	private CheckBox fave;

	public TrackListView(Context context, TrackList trackList) {
		super(context);

		/*
		 * First Icon and the Text to the right (horizontal), not above and
		 * below (vertical)
		 */
		 this.setOrientation(HORIZONTAL);

		// mIcon = new ImageView(context);
		// mIcon.setImageBitmap(aIconifiedText.getIcon());
		// left, top, right, bottom
		// mIcon.setPadding(0, 2, 5, 0); // 5px to the right

		/*
		 * At first, add the Icon to ourself (! we are extending LinearLayout)
		 */
		// addView(mIcon, new LinearLayout.LayoutParams(
		// LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		
		mText = new TextView(context);
		trackNum = new TextView(context);
		time = new TextView(context);
		fave = new CheckBox(context);
		
		mText.setText(trackList.getText());
		//mText.setTextSize(17);
		trackNum.setText(trackList.getTrackNum());
		time.setText(trackList.getTime());
		fave.setChecked(false);
		
		fave.setFocusable(false);
		mText.setFocusable(false);
		trackNum.setFocusable(false);
		time.setFocusable(false);
		
		
		LayoutParams layout =new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT);
		LayoutParams rightlayout =new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT); 
		rightlayout.weight = 0.69f;
		
		/* Now the text (after the icon) */
		addView(trackNum, layout);
		addView(mText, rightlayout);
		addView(time, layout);
		addView(fave, layout);
		
		
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

	public void setFave(boolean fave2) {
		fave.setChecked(fave2);
	}

	
	
	
}
