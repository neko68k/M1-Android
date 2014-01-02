package com.neko68k.M1;

import android.app.Activity;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;

public class GameListOptionsActivity extends Fragment{
	private MultiSelectSpinner cpulist;
	private MultiSelectSpinner sound1list;
	private MultiSelectSpinner sound2list;
	private MultiSelectSpinner sound3list;
	private MultiSelectSpinner sound4list;
	private MultiSelectSpinner yearlist;
	private MultiSelectSpinner boardlist;
	private MultiSelectSpinner mfglist;
	private CheckBox		   filterEnabled;	
	
	
	@Override
	public void onPause(){
		super.onPause();
		listClosed();
		return;
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		// Inflate the layout for this fragment
		View view = inflater.inflate(R.layout.gamelistsortoptions, container,
				false);

		// get controls
		cpulist = (MultiSelectSpinner) view.findViewById(R.id.CPU);
		sound1list = (MultiSelectSpinner) view.findViewById(R.id.Sound1);
		sound2list = (MultiSelectSpinner) view.findViewById(R.id.Sound2);
		sound3list = (MultiSelectSpinner) view.findViewById(R.id.Sound3);
		sound4list = (MultiSelectSpinner) view.findViewById(R.id.Sound4);
		yearlist = (MultiSelectSpinner) view.findViewById(R.id.Year);
		mfglist = (MultiSelectSpinner) view.findViewById(R.id.Mfg);
		boardlist = (MultiSelectSpinner) view.findViewById(R.id.Board);
		filterEnabled = (CheckBox) view.findViewById(R.id.EnableFilter);
		
		//boardlist.getItemIdAtPosition(position);
		
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		Cursor cpuCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.CPU_TABLE, GameListOpenHelper.KEY_CPU);
		cpulist.setItems(cpuCursor, GameListOpenHelper.KEY_CPU);

		//cpulist.setOnItemSelectedListener(this);

		Cursor sound1Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND1_TABLE, GameListOpenHelper.KEY_SOUND1);
		sound1list.setItems(sound1Cursor, GameListOpenHelper.KEY_SOUND1);

		Cursor sound2Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND2_TABLE, GameListOpenHelper.KEY_SOUND2);
		sound2list.setItems(sound2Cursor, GameListOpenHelper.KEY_SOUND2);

		Cursor sound3Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND3_TABLE, GameListOpenHelper.KEY_SOUND3);
		sound3list.setItems(sound3Cursor, GameListOpenHelper.KEY_SOUND3);

		Cursor sound4Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND4_TABLE, GameListOpenHelper.KEY_SOUND4);
		sound4list.setItems(sound4Cursor, GameListOpenHelper.KEY_SOUND4);
		
		Cursor mfgCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.MFG_TABLE, GameListOpenHelper.KEY_MFG);
		mfglist.setItems(mfgCursor, GameListOpenHelper.KEY_MFG);
		
		Cursor boardCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.BOARD_TABLE, GameListOpenHelper.KEY_SYS);
		boardlist.setItems(boardCursor, GameListOpenHelper.KEY_SYS);
		
		Cursor yearCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.YEAR_TABLE, GameListOpenHelper.KEY_YEAR);
		yearlist.setItems(yearCursor, GameListOpenHelper.KEY_YEAR);
		
		
		
		db.close();
		
		
		return view;
	}
	
	OnOptionsChanged mCallback;
	
	public interface OnOptionsChanged {
		public void onOptionsChanged(Bundle b);
	}
	
	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);

		// This makes sure that the container activity has implemented
		// the callback interface. If not, it throws an exception
		try {
			mCallback = (OnOptionsChanged) activity;
		} catch (ClassCastException e) {
			throw new ClassCastException(activity.toString()
					+ " must implement OnOptionsChanged");
		}
	}

	public void listClosed() {
		Bundle b = new Bundle();
		b.putBooleanArray("cpu", cpulist.getSelectedIndicies());
		b.putBooleanArray("sound1", sound1list.getSelectedIndicies());
		b.putBooleanArray("sound2", sound2list.getSelectedIndicies());
		b.putBooleanArray("sound3", sound3list.getSelectedIndicies());
		b.putBooleanArray("sound4", sound4list.getSelectedIndicies());
		b.putBooleanArray("mfg", mfglist.getSelectedIndicies());
		b.putBooleanArray("board", boardlist.getSelectedIndicies());
		b.putBooleanArray("year", yearlist.getSelectedIndicies());
		mCallback.onOptionsChanged(b);
		// TODO Auto-generated method stub
		return;
	}
}
