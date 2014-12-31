package com.neko68k.M1;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.R;
import android.content.Context;
import android.os.Environment;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.Toast;

public class FileBrowser{ 

	
	private List<String> directoryEntries = new ArrayList<String>();
	private File currentDirectory = Environment.getExternalStorageDirectory();

	ArrayAdapter<String> directoryList;
	private int savenum;
	Context ctx;
	//final ListView lv;

	/** Called when the activity is first created. */
	public FileBrowser(Context ictx) {
		//super(ictx);
		ctx = ictx;
		
		savenum = 0;
		
		//lv = this.getListView();
		
		
		
		/*lv.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
			public boolean onItemLongClick(AdapterView<?> av, View v,
					int pos, long id) {
				onListItemLongClick(lv, v, pos, id);
				return false;
			}
		});*/
		//lv.setAdapter(directoryList);
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
		//String state = Environment.getExternalStorageState();

		// browseTo(Environment.getExternalStorageDirectory());
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
		// this.directoryEntries.add(".");
		// String test = this.currentDirectory.getAbsolutePath();
		if (this.currentDirectory.getParent() != null)
			// if(this.currentDirectory.getParent()!="/") //TODO: fix this
			this.directoryEntries.add("..");

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
		
		//lv.setAdapter(directoryList);
		//this.setListAdapter(directoryList);
	}
	
	public List<String> getEntries(){
		return (directoryEntries);
	}
	

	/*protected void onListItemLongClick(ListView l, View v, int position, long id) {

		String fn = new String(this.currentDirectory.getAbsolutePath());

		int selectionRowID = (int) position;// (int)
											// this.getSelectedItemPosition();
		String selectedFileString = this.directoryEntries.get(selectionRowID);

		if (!selectedFileString.equals("..") && !selectedFileString.equals(".")) {
			File clickedFile = null;
			switch (this.displayMode) {
			case RELATIVE:
				
			case ABSOLUTE:
				try {
					clickedFile = new File(
							this.directoryEntries.get(selectionRowID));
				} catch (SecurityException e) {
					clickedFile = null;
				}
				break;
			}
			if (clickedFile != null && clickedFile.isDirectory()) {
				fn = clickedFile.getAbsolutePath();
				Intent i = new Intent().putExtra("com.neko68k.M1.FN", fn);
				i.putExtra("savenum", savenum);
				// startActivity(i);
				//setResult(RESULT_OK, i);
				//finish();
			}
		}
	}*/

	
	
}