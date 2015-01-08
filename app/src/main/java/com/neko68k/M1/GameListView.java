package com.neko68k.M1;

import android.content.Context;
import android.graphics.Bitmap;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class GameListView extends LinearLayout {
	private TextView mText;
	private ImageView mIcon;

	public GameListView(Context context, GameList aIconifiedText) {
		super(context);

		/*
		 * First Icon and the Text to the right (horizontal), not above and
		 * below (vertical)
		 */
		// this.setOrientation(HORIZONTAL);

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
		mText.setText(aIconifiedText.getText());
		mText.setTextSize(17);
		/* Now the text (after the icon) */
		addView(mText, new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT));
		this.setFocusable(true);
		
	}

	public void setText(String words) {
		mText.setText(words);
	}

	public void setIcon(Bitmap bullet) {
		mIcon.setImageBitmap(bullet);
	}
	
	
	
}
