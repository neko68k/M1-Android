package com.neko68k.M1;

import android.app.AlertDialog;
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
		Preference pref;
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences);
		browser = new FileBrowser(getApplicationContext());
		pref = this.getPreferenceManager().findPreference("romdir");
		pref.setSummary(pref.getSharedPreferences().getString("romdir", ""));
		pref = this.getPreferenceManager().findPreference("sysdir");
		pref.setSummary(pref.getSharedPreferences().getString("sysdir", ""));
		pref = this.getPreferenceManager().findPreference("icondir");
		pref.setSummary(pref.getSharedPreferences().getString("icondir", ""));
		setResult(RESULT_OK);

	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			final Preference preference) {
		if (preference.getKey().equals("romdir")||preference.getKey().equals("sysdir")||preference.getKey().equals("icondir")) {
			browser.showBrowserDlg(preference);
		}
		return true;
	}

	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		
		return true;
	}
}
