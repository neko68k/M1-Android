package com.neko68k.M1;

import android.content.Context;
import android.graphics.Bitmap;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class OptionsListView extends LinearLayout{
	private TextView mText;
    private ImageView mIcon;
   
    public OptionsListView(Context context, OptionsList aIconifiedText) {
            super(context);

            /* First Icon and the Text to the right (horizontal),
             * not above and below (vertical) */
            this.setOrientation(HORIZONTAL);


           
            mText = new TextView(context);
            mText.setText(aIconifiedText.getText());
            /* Now the text (after the icon) */
            addView(mText, new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
    }

    public void setText(String words) {
            mText.setText(words);
    }
   
    public void setIcon(Bitmap bullet) {
            mIcon.setImageBitmap(bullet);
    }
}
