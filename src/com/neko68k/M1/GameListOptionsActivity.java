package com.neko68k.M1;

import java.util.Iterator;
import java.util.List;

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
		
		if(GameListFragment.isFiltered()){
			filterEnabled.setChecked(true);
		}
		
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		Cursor cpuCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.CPU_TABLE, GameListOpenHelper.KEY_CPU);
		cpulist.setItems(cpuCursor, GameListOpenHelper.KEY_CPU, GameListOpenHelper.KEY_FILTERED);

		Cursor sound1Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND1_TABLE, GameListOpenHelper.KEY_SOUND1);
		sound1list.setItems(sound1Cursor, GameListOpenHelper.KEY_SOUND1, GameListOpenHelper.KEY_FILTERED);
		
		Cursor sound2Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND2_TABLE, GameListOpenHelper.KEY_SOUND2);
		sound2list.setItems(sound2Cursor, GameListOpenHelper.KEY_SOUND2, GameListOpenHelper.KEY_FILTERED);

		Cursor sound3Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND3_TABLE, GameListOpenHelper.KEY_SOUND3);
		sound3list.setItems(sound3Cursor, GameListOpenHelper.KEY_SOUND3, GameListOpenHelper.KEY_FILTERED);

		Cursor sound4Cursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.SOUND4_TABLE, GameListOpenHelper.KEY_SOUND4);
		sound4list.setItems(sound4Cursor, GameListOpenHelper.KEY_SOUND4, GameListOpenHelper.KEY_FILTERED);
		
		Cursor mfgCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.MFG_TABLE, GameListOpenHelper.KEY_MFG);
		mfglist.setItems(mfgCursor, GameListOpenHelper.KEY_MFG, GameListOpenHelper.KEY_FILTERED);
		
		Cursor boardCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.BOARD_TABLE, GameListOpenHelper.KEY_SYS);
		boardlist.setItems(boardCursor, GameListOpenHelper.KEY_SYS, GameListOpenHelper.KEY_FILTERED);
		
		Cursor yearCursor = GameListOpenHelper.getAllExtra(db, GameListOpenHelper.YEAR_TABLE, GameListOpenHelper.KEY_YEAR);
		yearlist.setItems(yearCursor, GameListOpenHelper.KEY_YEAR, GameListOpenHelper.KEY_FILTERED);
		
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
		
		// *************
		// 
		
		GameListOpenHelper.resetExtras(GameListOpenHelper.MFG_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.YEAR_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.BOARD_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND1_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND2_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND3_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND4_TABLE);
		GameListOpenHelper.resetExtras(GameListOpenHelper.CPU_TABLE);
		
		List<String> cpu = cpulist.getSelectedStrings();
		List<String> sound1 = sound1list.getSelectedStrings();
		List<String> sound2 = sound2list.getSelectedStrings();
		List<String> sound3 = sound3list.getSelectedStrings();
		List<String> sound4 = sound4list.getSelectedStrings();
		List<String> mfg = mfglist.getSelectedStrings();
		List<String> board = boardlist.getSelectedStrings();
		List<String> year = yearlist.getSelectedStrings();
		
		//cpu.toA

		GameListOpenHelper.updateExtra(GameListOpenHelper.CPU_TABLE, GameListOpenHelper.KEY_CPU, cpu.toArray(new String[cpu.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.SOUND1_TABLE, GameListOpenHelper.KEY_SOUND1, sound1.toArray(new String[sound1.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.SOUND2_TABLE, GameListOpenHelper.KEY_SOUND2, sound2.toArray(new String[sound2.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.SOUND3_TABLE, GameListOpenHelper.KEY_SOUND3, sound3.toArray(new String[sound3.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.SOUND4_TABLE, GameListOpenHelper.KEY_SOUND4, sound4.toArray(new String[sound4.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.MFG_TABLE, GameListOpenHelper.KEY_MFG, mfg.toArray(new String[mfg.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.BOARD_TABLE, GameListOpenHelper.KEY_SYS, board.toArray(new String[board.size()]));
		GameListOpenHelper.updateExtra(GameListOpenHelper.YEAR_TABLE, GameListOpenHelper.KEY_YEAR, year.toArray(new String[year.size()]));
				
		//b.putBooleanArray("cpu",  cpulist.getSelectedIndicies());
		//b.putBooleanArray("sound1",  sound1list.getSelectedIndicies());
		//b.putBooleanArray("sound2",  sound2list.getSelectedIndicies());
		//b.putBooleanArray("sound3",  sound3list.getSelectedIndicies());
		//b.putBooleanArray("sound4",  sound4list.getSelectedIndicies());
		//b.putBooleanArray("mfg",  mfglist.getSelectedIndicies());
		//b.putBooleanArray("board",  boardlist.getSelectedIndicies());
		//b.putBooleanArray("year",  yearlist.getSelectedIndicies());
		
		GameListOpenHelper.cpulist = cpu.size();
		GameListOpenHelper.sound1list = sound1.size();
		GameListOpenHelper.sound2list = sound2.size();
		GameListOpenHelper.sound3list = sound3.size();
		GameListOpenHelper.sound4list = sound4.size();
		GameListOpenHelper.mfglist = mfg.size();
		GameListOpenHelper.boardlist = board.size();
		GameListOpenHelper.yearlist = year.size();
		
		b.putBoolean("filtered", filterEnabled.isChecked());
		mCallback.onOptionsChanged(b);
		// TODO Auto-generated method stub
		return;
	}
}






















