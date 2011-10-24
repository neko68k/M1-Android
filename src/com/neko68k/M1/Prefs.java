package com.neko68k.M1;

import android.os.Bundle;
import android.preference.PreferenceActivity;

import com.neko68k.emu.M1Android.R;

public class Prefs extends PreferenceActivity {
	@Override
    protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.preferences);            
                        
            setResult(RESULT_OK);
            
    }
}
