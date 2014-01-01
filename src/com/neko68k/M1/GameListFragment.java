package com.neko68k.M1;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

public class GameListFragment extends FragmentActivity implements
		GameListActivity.OnItemSelectedListener, GameListOptionsActivity.OnOptionsChanged {

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.gamelistfraglayout);

		// Check that the activity is using the layout version with
		// the fragment_container FrameLayout
		if (findViewById(R.id.fragment_container) != null) {

			// However, if we're being restored from a previous state,
			// then we don't need to do anything and should return or else
			// we could end up with overlapping fragments.
			if (savedInstanceState != null) {
				return;
			}

			// Create a new Fragment to be placed in the activity layout
			GameListActivity firstFragment = new GameListActivity();

			// In case this activity was started with special instructions from
			// an
			// Intent, pass the Intent's extras to the fragment as arguments
			// firstFragment.setArguments(getIntent().getExtras());

			// Add the fragment to the 'fragment_container' FrameLayout
			getSupportFragmentManager().beginTransaction()
					.add(R.id.fragment_container, firstFragment).commit();
		}

	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		switch (item.getItemId()) {
		case R.id.sortOptions:
			GameListOptionsActivity newFragment = new GameListOptionsActivity();
			// args.putInt(GameListOptionsActivity.ARG_POSITION, position);
			// newFragment.setArguments(args);

			FragmentTransaction transaction = getSupportFragmentManager()
					.beginTransaction();

			// Replace whatever is in the fragment_container view with this
			// fragment,
			// and add the transaction to the back stack so the user can
			// navigate back
			transaction.replace(R.id.fragment_container, newFragment);
			transaction.addToBackStack(null);

			// Commit the transaction
			transaction.commit();

		default:

			return super.onOptionsItemSelected(item);
		}
	}
	
	public void onOptionsChanged(Bundle b){
		boolean[] cpulist = b.getBooleanArray("cpu");
		boolean[] sound1list = b.getBooleanArray("sound1");
		boolean[] sound2list = b.getBooleanArray("sound2");
		boolean[] sound3list = b.getBooleanArray("sound3");
		boolean[] sound4list = b.getBooleanArray("sound4");
		boolean[] mfglist = b.getBooleanArray("mfg");
		boolean[] boardlist = b.getBooleanArray("board");
		boolean[] yearlist = b.getBooleanArray("year");
		
		return;
	}

	public void onGameSelected(Game game) {
		// //Game game = NDKBridge.queryRom(position);

		// LoadROMTask loadTask = new LoadROMTask(this);
		// loadTask.execute(new Integer(selectionRowID));
		// game.index= position;
		Intent i = new Intent();
		String title = "";// NDKBridge.getGameList(selectionRowID);
		i.putExtra("com.neko68k.M1.title", title);
		i.putExtra("com.neko68k.M1.position", game.index);
		i.putExtra("com.neko68k.M1.game", game);
		// startActivity(i);
		setResult(RESULT_OK, i);
		finish();

	}
}
