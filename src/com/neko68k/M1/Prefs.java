package com.neko68k.M1;

import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.widget.ListView;

public class Prefs extends PreferenceActivity implements
		Preference.OnPreferenceChangeListener {
	
	FileBrowser browser;
	AlertDialog dialog;
	ListView list;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Preference romdirpref;
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences);
		browser = new FileBrowser(getApplicationContext());
		romdirpref = this.getPreferenceManager().findPreference("romdir");
		romdirpref.setSummary(romdirpref.getSharedPreferences().getString("romdir", ""));
		setResult(RESULT_OK);

	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			final Preference preference) {
		if (preference.getKey().equals("romdir")||preference.getKey().equals("basedir")) {
			browser.showBrowserDlg(preference);
		}
		return true;
	}

	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		return true;
	}
}
