package com.neko68k.M1;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.view.View;
import android.widget.ImageView;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

public class GameListCursorAdapter extends SimpleCursorAdapter{
	//final int tblIcon;
	final int tblYear;
	final int tblMfg;
	final int tblBoard;
	final int tblHardware;
	final int tblRomname;
	
	static class ViewHolder{
		ImageView icon;
		TextView year;
		TextView mfg;
		TextView board;
		TextView hardware;
		
		
	}
	public GameListCursorAdapter(Context context, int layout, Cursor cursor,
			String[] from, int[] to) {
		super(context, layout, cursor, from, to);
		//tblIcon = cursor.getColumnIndexOrThrow("icon");
		tblYear = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_YEAR);
		tblMfg = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_MFG);
		tblBoard = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SYS);
		tblHardware = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_CPU);
		tblRomname = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_ROMNAME);
		
		// TODO Auto-generated constructor stub
	}
	
	
	@Override
	public void bindView(View view, Context context, Cursor cursor) {
		// TODO Auto-generated method stub
		
		ViewHolder holder = (ViewHolder) view.getTag();
		if(holder==null){
			holder = new ViewHolder();
			holder.icon = (ImageView) view.findViewById(R.id.icon);
			holder.year = (TextView) view.findViewById(R.id.year);
			holder.mfg = (TextView) view.findViewById(R.id.mfg);
			holder.board = (TextView) view.findViewById(R.id.board);
			holder.hardware = (TextView) view.findViewById(R.id.hardware);
			
			
			view.setTag(holder);
		}
		
		holder.year.setText(cursor.getString(tblYear));
		holder.mfg.setText(cursor.getString(tblMfg));
		holder.board.setText(cursor.getString(tblBoard));
		holder.hardware.setText(cursor.getString(tblHardware));
		//KEY_ROMNAME
		String romname = cursor.getString(tblRomname);
		File file = new File(NDKBridge.basepath+"/m1/icons/"+romname+".ico");
		FileInputStream inputStream;
		Bitmap bm;
		try{
            inputStream = new FileInputStream(file);
            bm = BitmapFactory.decodeStream(inputStream);
            holder.icon.setImageBitmap(Bitmap.createScaledBitmap(bm, 128, 128, false));
            }
            catch (FileNotFoundException e)
            {
                    e.printStackTrace();
            }
		
	//	read();
		
		
		super.bindView(view, context, cursor);
	}
	
	
	
}
