package com.neko68k.M1;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.Map;
import java.util.Timer;

/**
 * Created by neko on 1/8/15.
 */
public class MainDetailFragment extends Fragment {
    TextView trackNum;
    TextView playTime;
    TextView board;
    TextView hardware;
    TextView mfg;
    TextView song;
    TextView title;
    TextView year;
    Timer updateTimer;
    ImageView icon;
    Map<String, ?> preferences;
    boolean inited = false;


    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        
        //TrackList item;
        View v = inflater.inflate(R.layout.detailsview, container, false);
        //trackList = (ListView) v.findViewById(R.id.listView1);

        trackNum = (TextView) v.findViewById(R.id.trackNum);
        playTime = (TextView) v.findViewById(R.id.playTime);
        board = (TextView) v.findViewById(R.id.board);
        hardware = (TextView) v.findViewById(R.id.hardware);
        mfg = (TextView) v.findViewById(R.id.mfg);
        song = (TextView) v.findViewById(R.id.song);
        title = (TextView) v.findViewById(R.id.title);
        year = (TextView) v.findViewById(R.id.year);
        icon = (ImageView) v.findViewById(R.id.icon);

        Context ctx = getActivity();
        NDKBridge.ctx = ctx;
        setHasOptionsMenu(true);
        if(savedInstanceState!=null)
            inited = savedInstanceState.getBoolean("inited");
        FirstRun(ctx);
        return v;
    }

    @Override
    public void onSaveInstanceState (Bundle outState){
        outState.putBoolean("inited", inited);
    }

   /* @Override
    public void onRestoreInstanceState (Bundle inState){
        inited = inState.getBoolean("inited");
    }*/

    private void FirstRun(final Context ctx) {
        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(ctx);
        preferences = prefs.getAll();
        //Context ctx = getApplicationContext();
        Boolean firstRun = prefs.getBoolean("firstRun", true);
        NDKBridge.basepath = prefs.getString("sysdir", null);
        NDKBridge.rompath = (String) preferences.get("romdir");
        NDKBridge.iconpath = (String) preferences.get("icondir");

        if (firstRun == null || firstRun == true || NDKBridge.basepath == null) {
            AlertDialog alert = new AlertDialog.Builder(ctx)
                    .setTitle("First Run")
                    .setMessage(
                            "It looks like this is your first run. Please choose where you'd like to install. "
                                    + "For example, select '/sdcard' and I will install to '/sdcard/m1'. "
                                    + "If you already have a folder with the XML, LST and ROMs, choose its parent. For example, if you have "
                                    + "'/sdcard/m1' choose '/sdcard'. "
                                    + "I will not overwrite any files in it. Custom ROM folders, etc can be set in the Options."
                                    + "\n - Structure is:"
                                    + "\n   .../m1/m1.xml"
                                    + "\n   .../m1/lists" + "\n   .../m1/roms")
                    .setPositiveButton("OK",
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                                    int whichButton) {
                                    FileBrowser browser = new FileBrowser(ctx);

									/* User clicked OK so do some stuff */
                                    browser.showBrowserDlg("basedir", ctx);
                                }
                            })
                    .setNegativeButton("Cancel",
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                                    int whichButton) {

									/* User clicked Cancel so do some stuff */
                                }
                            }).create();
            alert.show();
        }
        else{
            if(!inited) {
                inited = true;
                InitM1Task task = new InitM1Task(NDKBridge.ctx);
                task.execute();
            }
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {

        inflater.inflate(R.menu.menu, menu);

    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Intent intent;
        InitM1Task task;
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.open:

                NDKBridge.loadError = false;
                intent = new Intent(NDKBridge.ctx, GameListActivity.class);
                startActivityForResult(intent, 1);
                return true;
            case R.id.options:
                intent = new Intent(NDKBridge.ctx, Prefs.class);
                startActivityForResult(intent, 2);
                return true;
            case R.id.rescan:
                SQLiteDatabase db = NDKBridge.m1db.getWritableDatabase();
                GameListOpenHelper.wipeTables(db);
                task = new InitM1Task(NDKBridge.ctx);
                task.execute();
                return true;

            default:

                return super.onOptionsItemSelected(item);
        }
    }
}
