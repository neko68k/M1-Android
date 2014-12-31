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
			Preference preference) {
		if (preference.getKey().equals("romdir")) {
            //Show your AlertDialog here!
        	final AlertDialog.Builder builder = new AlertDialog.Builder(this);

        	// 2. Chain together various setter methods to set the dialog characteristics
        	builder.setTitle("Select Folder");
        	
        	list = new ListView(this);
        	//list.setAdapter(new ArrayAdapter<String>(this, android.R.layout.select_dialog_item, items));
        	list.setOnItemClickListener(new OnItemClickListener() {
        	        //public void onItemClick(AdapterView<?> arg0, View view, int pos, long id) {
        	            
        	        //}

					public void onItemClick(AdapterView<?> arg0, View arg1,
							int arg2, long arg3) {
						String which = (String) (list.getItemAtPosition(arg2));
						String selectedFileString = which;//browser.getStringAtOfs(which);
						if (selectedFileString.equals(".")) {
							String selectedPath = browser.getCurrent();
							//((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
						} else if (selectedFileString.equals("..")) {
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
        	
        	
        	
        	list.setAdapter(browser.getAdapter());/*, 
        			new DialogInterface.OnClickListener() {
				
				public void onClick(DialogInterface dialog, int which) {
					String selectedFileString = browser.getStringAtOfs(which);
					if (selectedFileString.equals(".")) {
						String selectedPath = browser.getCurrent();
						//((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
					} else if (selectedFileString.equals("..")) {
						browser.upOneLevel();
						((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
					} else {
						File clickedFile = null;
						clickedFile = new File(browser.getCurrent()+selectedFileString);		
						if (clickedFile != null){
							browser.browseTo(clickedFile);
							((AlertDialog) dialog).getListView().setAdapter(browser.getAdapter());
							
						}
					}
				}});*/
			

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
