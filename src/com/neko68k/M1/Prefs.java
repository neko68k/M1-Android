package com.neko68k.M1;

import java.io.File;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class Prefs extends PreferenceActivity implements
		Preference.OnPreferenceChangeListener {
	
	private Context ctx;
	FileBrowser browser;
	AlertDialog dialog;
	ListView list;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences);
		browser = new FileBrowser(getApplicationContext());
		setResult(RESULT_OK);

	}
	
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        
        
    }

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			final Preference preference) {
		if (preference.getKey().equals("romdir")) {
            //Show your AlertDialog here!
        	final AlertDialog.Builder builder = new AlertDialog.Builder(this);

        	// 2. Chain together various setter methods to set the dialog characteristics
        	builder.setTitle("Select Folder");
        	//preference.set
        	list = new ListView(this);
        	list.setOnItemClickListener(new OnItemClickListener() {

					public void onItemClick(AdapterView<?> arg0, View arg1,
							int arg2, long arg3) {
						String which = (String) (list.getItemAtPosition(arg2));
						String selectedFileString = which;//browser.getStringAtOfs(which);
						if (selectedFileString.equals("(Select this folder)")) {
							String selectedPath = browser.getCurrent();
							if(selectedPath!=null){
								SharedPreferences sp = preference.getSharedPreferences();
								SharedPreferences.Editor sped = sp.edit();
								sped.putString("romdir", selectedPath);
								sped.commit();	
								dialog.dismiss();
							}
							//((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
						} else if (selectedFileString.equals("(Go up)")) {
							browser.upOneLevel();
							//((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
						} else {
							File clickedFile = null;
							clickedFile = new File(browser.getCurrent()+selectedFileString);		
							if (clickedFile != null){
								browser.browseTo(clickedFile);
								//((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
								
							}
						}
					}
        	    });
        	builder.setView(list);
        	       	list.setAdapter(browser.getAdapter());

        	// 3. Get the AlertDialog from create()
        	dialog = builder.create();
        	dialog.show();
        }
		return true;
	}

	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		return true;
	}
}
