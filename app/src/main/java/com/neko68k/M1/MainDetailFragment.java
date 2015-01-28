package com.neko68k.M1;

import android.app.Activity;
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
        if(savedInstanceState!=null) {
            inited = savedInstanceState.getBoolean("inited");
            NDKBridge.inited = inited;
            NDKBridge.game = savedInstanceState.getParcelable("game");
            if(inited&&NDKBridge.game!=null)
                updateDetails();
        }
        FirstRun(ctx);
        return v;
    }

    public void updateTrack(){
        int cmdNum = NDKBridge.getInfoInt(
                NDKBridge.M1_IINF_TRACKCMD, (NDKBridge
                        .getInfoInt(
                                NDKBridge.M1_IINF_CURSONG,
                                0) << 16 | NDKBridge
                        .getInfoInt(
                                NDKBridge.M1_IINF_CURGAME,
                                0)));
        String text = NDKBridge.getInfoStr(
                NDKBridge.M1_SINF_TRKNAME,
                cmdNum << 16
                        | NDKBridge.getInfoInt(
                        NDKBridge.M1_IINF_CURGAME,
                        0));
        int cursong = NDKBridge.getInfoInt(
                NDKBridge.M1_IINF_CURSONG, 0) + 1;// M1_IINF_CURSONG;
        song.setText("Title: " + text);// +track);
        trackNum.setText("Track: " + cursong);
    }

    public void updateDetails(){
        icon.setImageBitmap(NDKBridge.getIcon());
        title.setText(NDKBridge.game.getTitle());
        board.setText("Board: " + NDKBridge.game.sys);
        mfg.setText("Maker: " + NDKBridge.game.mfg);
        year.setText("Year: " + NDKBridge.game.year);
        hardware.setText("Hardware: " + NDKBridge.game.soundhw);



    }

    /*
    private Runnable mUpdateTimeTask = new Runnable() {
        public void run() {
            // update stuff here
            ((Activity) NDKBridge.ctx).runOnUiThread(new Runnable() {

                public void run() {
                    Integer cursong = 0;
                    if (playing == true) {
                        if (paused == false) {
                            int seconds = NDKBridge.getCurTime() / 60;
                            if (seconds > NDKBridge.songLen) {
                                trackList.smoothScrollToPosition(NDKBridge
                                        .next());
                                if (listLen)
                                    NDKBridge.getSongLen();
                                else
                                    NDKBridge.songLen = NDKBridge.defLen;
                                updateRemoteMetadata();
                            }
                            int minutes = seconds / 60;
                            seconds -= minutes * 60;
                            cursong = NDKBridge.getInfoInt(
                                    NDKBridge.M1_IINF_CURSONG, 0) + 1;// M1_IINF_CURSONG;
                            trackNum.setText("Track: " + cursong);
                            String tmp;
                            if (NDKBridge.songLen % 60 < 10)
                                tmp = ":0";
                            else
                                tmp = ":";
                            if (seconds < 10) {

                                playTime.setText("Time: " + minutes + ":0"
                                        + seconds + "/" + NDKBridge.songLen
                                        / 60 + tmp + NDKBridge.songLen % 60);
                            } else
                                playTime.setText("Time: " + minutes + ":"
                                        + seconds + "/" + NDKBridge.songLen
                                        / 60 + tmp + NDKBridge.songLen % 60);

                            // int game = m1snd_get_info_int(M1_IINF_CURGAME,
                            // 0);

                            // jstring track;

                            // cmdNum =
                            // m1snd_get_info_int(M1_IINF_TRACKCMD,(song<<16|game));
                            // __android_log_print(ANDROID_LOG_INFO,
                            // "M1Android", "Cmd: %i", cmdNum);

                            // String track =
                            // NDKBridge.getInfoStr(NDKBridge.M1_SINF_TRKNAME,
                            // cursong<<16|NDKBridge.curGame);
                            int cmdNum = NDKBridge.getInfoInt(
                                    NDKBridge.M1_IINF_TRACKCMD, (NDKBridge
                                            .getInfoInt(
                                                    NDKBridge.M1_IINF_CURSONG,
                                                    0) << 16 | NDKBridge
                                            .getInfoInt(
                                                    NDKBridge.M1_IINF_CURGAME,
                                                    0)));
                            String text = NDKBridge.getInfoStr(
                                    NDKBridge.M1_SINF_TRKNAME,
                                    cmdNum << 16
                                            | NDKBridge.getInfoInt(
                                            NDKBridge.M1_IINF_CURGAME,
                                            0));
                            song.setText("Title: " + text);// +track);
                            //if (mRemoteControlClientCompat != null)

                        }
                    }
                }
            });
            // retrigger task
            if (playing == true) {
                mHandler.postDelayed(mUpdateTimeTask, 100);
            }
        }
    };*/

    @Override
    public void onSaveInstanceState (Bundle outState){
        outState.putBoolean("inited", inited);
        outState.putParcelable("game", NDKBridge.game);
    }

    /*@Override
    public void onPause(){

        int test;
        test = 1;
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
            } else {

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
