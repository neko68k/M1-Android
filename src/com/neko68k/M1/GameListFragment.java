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

	private static boolean filtered = false;
	
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
		filtered = b.getBoolean("filtered");
		if(!filtered){
			GameListOpenHelper.resetExtras(GameListOpenHelper.MFG_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.YEAR_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.BOARD_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND1_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND2_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND3_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.SOUND4_TABLE);
			GameListOpenHelper.resetExtras(GameListOpenHelper.CPU_TABLE);
		}
		
		return;
	}
	
	public static boolean isFiltered(){
		return filtered;
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
		//i.putExtra("com.neko68k.M1.game", game);
		// startActivity(i);
		NDKBridge.game = game;
		setResult(RESULT_OK, i);
		finish();

	}
}
