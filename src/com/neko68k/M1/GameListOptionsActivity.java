package com.neko68k.M1;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;

public class GameListOptionsActivity extends Fragment implements OnItemSelectedListener{
	private MultiSelectSpinner cpulist;
	private MultiSelectSpinner sound1list;
	private MultiSelectSpinner sound2list;
	private MultiSelectSpinner sound3list;
	private MultiSelectSpinner sound4list;
	private MultiSelectSpinner yearlist;
	private MultiSelectSpinner boardlist;
	private MultiSelectSpinner mfglist;
	
	public void onItemSelected(AdapterView<?> a, View v, int i, long l){
		//CheckBox chk = (CheckBox)v.findViewById(R.id.chkbox);
		//boolean test = chk.isChecked();
		return;
	}
	public void onNothingSelected(AdapterView<?> a){
		
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
		
		
		//boardlist.getItemIdAtPosition(position);
		
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		Cursor cpuCursor = GameListOpenHelper.getAllCPU(db);
		cpulist.setItems(cpuCursor, GameListOpenHelper.KEY_CPU);

		//cpulist.setOnItemSelectedListener(this);

		Cursor sound1Cursor = GameListOpenHelper.getAllSound1(db);
		sound1list.setItems(sound1Cursor, GameListOpenHelper.KEY_SOUND1);

		Cursor sound2Cursor = GameListOpenHelper.getAllSound2(db);
		sound2list.setItems(sound2Cursor, GameListOpenHelper.KEY_SOUND2);

		Cursor sound3Cursor = GameListOpenHelper.getAllSound3(db);
		sound3list.setItems(sound3Cursor, GameListOpenHelper.KEY_SOUND3);

		Cursor sound4Cursor = GameListOpenHelper.getAllSound4(db);
		sound4list.setItems(sound4Cursor, GameListOpenHelper.KEY_SOUND4);
		
		Cursor mfgCursor = GameListOpenHelper.getAllMfg(db);
		mfglist.setItems(mfgCursor, GameListOpenHelper.KEY_MFG);
		
		Cursor boardCursor = GameListOpenHelper.getAllBoard(db);
		boardlist.setItems(boardCursor, GameListOpenHelper.KEY_SYS);
		
		Cursor yearCursor = GameListOpenHelper.getAllYear(db);
		yearlist.setItems(yearCursor, GameListOpenHelper.KEY_YEAR);
		
		
		db.close();
		return view;
	}
}
