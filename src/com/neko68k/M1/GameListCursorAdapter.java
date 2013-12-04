package com.neko68k.M1;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.view.View;
import android.widget.AlphabetIndexer;
import android.widget.ImageView;
import android.widget.SectionIndexer;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

public class GameListCursorAdapter extends SimpleCursorAdapter implements
		SectionIndexer {
	// final int tblIcon;
	final int tblYear;
	final int tblMfg;
	final int tblBoard;
	final int tblHardware;
	final int tblRomname;
	final int tblId;

	AlphabetIndexer mAlphabetIndexer;

	static class ViewHolder {
		ImageView icon;
		TextView year;
		TextView mfg;
		TextView board;
		TextView hardware;
		String romname;
		int index;
		int position;

	}

	public GameListCursorAdapter(Context context, int layout, Cursor cursor,
			String[] from, int[] to) {
		super(context, layout, cursor, from, to);
		tblYear = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_YEAR);
		tblMfg = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_MFG);
		tblBoard = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SYS);
		tblHardware = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_CPU);
		tblRomname = cursor
				.getColumnIndexOrThrow(GameListOpenHelper.KEY_ROMNAME);
		tblId = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_ID);

		mAlphabetIndexer = new AlphabetIndexer(cursor,
				cursor.getColumnIndex(GameListOpenHelper.KEY_TITLE),
				" ABCDEFGHIJKLMNOPQRTSUVWXYZ");
		mAlphabetIndexer.setCursor(cursor);// Sets a new cursor as the data set
											// and resets the cache of indices.
		// TODO Auto-generated constructor stub
	}

	/**
	 * Performs a binary search or cache lookup to find the first row that
	 * matches a given section's starting letter.
	 */

	public int getPositionForSection(int sectionIndex) {
		return mAlphabetIndexer.getPositionForSection(sectionIndex);
	}

	/**
	 * Returns the section index for a given position in the list by querying
	 * the item and comparing it with all items in the section array.
	 */

	public int getSectionForPosition(int position) {
		return mAlphabetIndexer.getSectionForPosition(position);
	}

	/**
	 * Returns the section array constructed from the alphabet provided in the
	 * constructor.
	 */

	public Object[] getSections() {
		return mAlphabetIndexer.getSections();
	}

	@Override
	public void bindView(View view, Context context, Cursor cursor) {
		// TODO Auto-generated method stub

		ViewHolder holder = (ViewHolder) view.getTag();
		if (holder == null) {
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
		holder.index = cursor.getInt(tblId);
		// KEY_ROMNAME
		holder.romname = cursor.getString(tblRomname);
		new AsyncTask<ViewHolder, Void, Bitmap>() {
			private ViewHolder v;

			@Override
			protected Bitmap doInBackground(ViewHolder... params) {
				Bitmap bm = null;
				v = params[0];
				File file = new File(NDKBridge.basepath + "/m1/icons/"
						+ v.romname + ".ico");
				FileInputStream inputStream;
				try {
					inputStream = new FileInputStream(file);
					bm = BitmapFactory.decodeStream(inputStream);
					inputStream.close();
					// v.icon.setImageBitmap(Bitmap.createScaledBitmap(bm, 128,
					// 128, false));
				} catch (FileNotFoundException e) {
					// check for parent(and clone) romsets
					// if none set default icon
					file = new File(NDKBridge.basepath
							+ "/m1/icons/"
							+ NDKBridge.getInfoStr(
									NDKBridge.M1_SINF_PARENTNAME, v.index)
							+ ".ico");
					try {
						inputStream = new FileInputStream(file);
						bm = BitmapFactory.decodeStream(inputStream);
						inputStream.close();

					} catch (FileNotFoundException e1) {
						// TODO Auto-generated catch block

						e1.printStackTrace();
						return (null);
					} catch (IOException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}

					e.printStackTrace();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				// v.icon.setImageBitmap(Bitmap.createScaledBitmap(bm, 128, 128,
				// false));

				return (Bitmap.createScaledBitmap(bm, 128, 128, false));
			}

			@Override
			protected void onPostExecute(Bitmap result) {
				super.onPostExecute(result);
				/*
				 * if (v.position == position) { // If this item hasn't been
				 * recycled already, hide the // progress and set and show the
				 * image //v.progress.setVisibility(View.GONE);
				 * v.icon.setVisibility(View.VISIBLE);
				 * v.icon.setImageBitmap(result); }
				 */

				v.icon.setImageBitmap(result);
			}
		}.execute(holder);

		/*
		 * File file = new File(NDKBridge.basepath+"/m1/icons/"+romname+".ico");
		 * FileInputStream inputStream; Bitmap bm; try{ inputStream = new
		 * FileInputStream(file); bm = BitmapFactory.decodeStream(inputStream);
		 * holder.icon.setImageBitmap(Bitmap.createScaledBitmap(bm, 128, 128,
		 * false)); } catch (FileNotFoundException e) { e.printStackTrace(); }
		 */

		// read();

		super.bindView(view, context, cursor);
	}

}
