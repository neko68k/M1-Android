package com.neko68k.M1;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnShowListener;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;



public class FileBrowser{ 
	private List<String> directoryEntries = new ArrayList<String>();
	private File currentDirectory = Environment.getExternalStorageDirectory();

	ArrayAdapter<String> directoryList;
	Context ctx;
	
	AlertDialog dialog;
	ListView list;
	
	public interface FBCallback{
		public void selected();
	}
	
	public void showBrowserDlg(final Preference preference){
    	final AlertDialog.Builder builder = new AlertDialog.Builder(preference.getContext());

    	builder.setTitle("Select Folder");
    	list = new ListView(preference.getContext());
    	list.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> arg0, View arg1,
						int arg2, long arg3) {
					String selectedFileString = (String) (list.getItemAtPosition(arg2));;
					if (selectedFileString.equals("(Select this folder)")) {
						String selectedPath = getCurrent();
						if(selectedPath!=null){
							SharedPreferences sp = preference.getSharedPreferences();
							SharedPreferences.Editor sped = sp.edit();
							sped.putString(preference.getKey(), selectedPath);
							sped.commit();	
							dialog.dismiss();
							preference.setSummary(selectedPath);
							return;
						}
					} else if (selectedFileString.equals("(Go up)")) {
						upOneLevel();
					} else {
						File clickedFile = null;
						clickedFile = new File(getCurrent()+selectedFileString);		
						if (clickedFile != null){
							browseTo(clickedFile);
						}
					}
				}
    	    });
    	builder.setView(list);
    	
    	list.setAdapter(directoryList);

    	dialog = builder.create();
    	dialog.show();
    
	}
	
	public void showBrowserDlg(final String key, final Context ictx){
		
		final FBCallback mCallback = (FBCallback)NDKBridge.ctx;
		
    	final AlertDialog.Builder builder = new AlertDialog.Builder(ictx);

    	builder.setTitle("Select Folder");
    	list = new ListView(ictx);
    	list.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> arg0, View arg1,
						int arg2, long arg3) {
					String selectedFileString = (String) (list.getItemAtPosition(arg2));;
					if (selectedFileString.equals("(Select this folder)")) {
						String selectedPath = getCurrent();
						if(selectedPath!=null){
							SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(ictx);//.getSharedPreferences();
							SharedPreferences.Editor sped = sp.edit();
							sped.putString("sysdir", selectedPath+"/m1");
							sped.putString("romdir",selectedPath+"/m1/roms");
							sped.putString("icondir",selectedPath+"/m1/icons/");
							sped.commit();	
							dialog.dismiss();
							mCallback.selected();
							
							return;
						}
					} else if (selectedFileString.equals("(Go up)")) {
						upOneLevel();
					} else {
						File clickedFile = null;
						clickedFile = new File(getCurrent()+selectedFileString);		
						if (clickedFile != null){
							browseTo(clickedFile);
						}
					}
				}
    	    });
    	builder.setView(list);
    	list.setAdapter(directoryList);
    	
    	dialog = builder.create();
    	dialog.show();
    
	}


	/** Called when the activity is first created. */
	public FileBrowser(Context ictx) {
		ctx = ictx;
		
		directoryList = new ArrayAdapter<String>(ctx,
				R.layout.simple_list_item_1, this.directoryEntries);

		browseToRoot();
	}
	
	public String getCurrent(){
			return (this.currentDirectory.getAbsolutePath());
	}
	
	public String getStringAtOfs(long l){
		return(this.directoryEntries.get((int) l));
	}
	
	public ArrayAdapter<String> getAdapter(){
		return(directoryList);
	}

	/**
	 * This function browses to the root-directory of the file-system.
	 */
	public void browseToRoot() {
		browseTo(new File("/mnt"));
	}

	/**
	 * This function browses up one level according to the field:
	 * currentDirectory
	 */
	public void upOneLevel() {
		if (this.currentDirectory.getParent() != null)
			this.browseTo(this.currentDirectory.getParentFile());
	}

	public void browseTo(final File aDirectory) {
		
		File files[] = null;
		if (aDirectory.isDirectory()) {
			this.currentDirectory = aDirectory;
			try {
				files = aDirectory.listFiles();
			} catch (SecurityException e) {
				Toast.makeText(ctx, "Permission denied.", Toast.LENGTH_SHORT)
						.show();
				return;
			}
			fill(files);

		}
	}

	private void fill(File[] files) {
		this.directoryEntries.clear();

		// Add the "." and the ".." == 'Up one level'
		try {
			Thread.sleep(10);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		this.directoryEntries.add("(Select this folder)");
		if (this.currentDirectory.getParent() != null)
			this.directoryEntries.add("(Go up)");

		// On relative Mode, we have to add the current-path to
						// the beginning
			int currentPathStringLength = this.currentDirectory
					.getAbsolutePath().length();
			for (File file : files) {
				if (file.isDirectory()) {
					this.directoryEntries.add(file.getAbsolutePath().substring(
							currentPathStringLength)
							+ "/");				
				}
			}
			
		Collections.sort(this.directoryEntries);
		directoryList.notifyDataSetChanged();
	}
	
	public List<String> getEntries(){
		return (directoryEntries);
	}	
}