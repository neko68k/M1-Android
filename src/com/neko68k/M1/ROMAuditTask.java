package com.neko68k.M1;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;

public class ROMAuditTask extends AsyncTask<Void, Void, Void> {
	ProgressDialog dialog;

	private Context context;

	public ROMAuditTask(Context c) // pass the context in the constructor
	{
		context = c;
	}

	public void setContext(Context c) {
		context = c;
	}

	@Override
	protected void onPreExecute() {
		dialog = ProgressDialog.show(context, "",
				"Auditing ROM sets. This may take a long time.", true);
	}

	@Override
	protected Void doInBackground(Void... unused) {
		// do stuff here
		int numGames = NDKBridge.getMaxGames();
		int i = 0;
		int j = 0;
		String title;

		// List<GameList> mItems = new ArrayList<GameList>();
		for (i = 0; i < numGames; i++) {
			// NDKBridge.auditROM(i);
		}

		return (null);
	}

	@Override
	protected void onPostExecute(Void result) {
		// NDKBridge.globalGLA = NDKCallbacks.nonglobalgla;
		dialog.dismiss();
	}
}