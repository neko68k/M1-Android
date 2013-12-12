package com.neko68k.M1;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.widget.SimpleCursorAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.Spinner;

public class GameListOptionsActivity extends Fragment {
	private Spinner cpulist;
	private Spinner sound1list;
	private Spinner sound2list;
	private Spinner sound3list;
	private Spinner sound4list;
	private Spinner yearlist;
	private Spinner boardlist;
	private Spinner mfglist;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		// Inflate the layout for this fragment
		View view = inflater.inflate(R.layout.gamelistsortoptions, container,
				false);

		// get controls
		cpulist = (Spinner) view.findViewById(R.id.CPU);
		sound1list = (Spinner) view.findViewById(R.id.Sound1);
		sound2list = (Spinner) view.findViewById(R.id.Sound2);
		sound3list = (Spinner) view.findViewById(R.id.Sound3);
		sound4list = (Spinner) view.findViewById(R.id.Sound4);
		yearlist = (Spinner) view.findViewById(R.id.Year);
		mfglist = (Spinner) view.findViewById(R.id.Mfg);
		boardlist = (Spinner) view.findViewById(R.id.Board);
		
		SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
		Cursor cpuCursor = GameListOpenHelper.getAllCPU(db);
		SimpleCursorAdapter sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, cpuCursor,
				new String[] { "_id","cpu" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		cpulist.setAdapter(sca);

		Cursor sound1Cursor = GameListOpenHelper.getAllSound1(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, sound1Cursor,
				new String[] { "_id","sound1" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		sound1list.setAdapter(sca);

		Cursor sound2Cursor = GameListOpenHelper.getAllSound2(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, sound2Cursor,
				new String[] { "_id","sound2" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		sound2list.setAdapter(sca);

		Cursor sound3Cursor = GameListOpenHelper.getAllSound3(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, sound3Cursor,
				new String[] { "_id", "sound3" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		sound3list.setAdapter(sca);

		Cursor sound4Cursor = GameListOpenHelper.getAllSound4(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, sound4Cursor,
				new String[] { "_id","sound4" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		sound4list.setAdapter(sca);
		
		Cursor mfgCursor = GameListOpenHelper.getAllMfg(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, mfgCursor,
				new String[] { "_id","mfg" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		mfglist.setAdapter(sca);
		
		Cursor boardCursor = GameListOpenHelper.getAllBoard(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, boardCursor,
				new String[] { "_id","sys" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		boardlist.setAdapter(sca);
		
		Cursor yearCursor = GameListOpenHelper.getAllYear(db);
		sca = new SimpleCursorAdapter(this.getActivity(),
				R.layout.option_spinner_layout, yearCursor,
				new String[] { "_id","year" }, new int[] { R.id.text_id, R.id.text_opt }, 0);
		yearlist.setAdapter(sca);
		
		db.close();
		return view;
	}
}
