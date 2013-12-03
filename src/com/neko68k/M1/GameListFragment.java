package com.neko68k.M1;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;

public class GameListFragment extends FragmentActivity implements GameListActivity.OnItemSelectedListener{
	
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
            
            // In case this activity was started with special instructions from an
            // Intent, pass the Intent's extras to the fragment as arguments
            //firstFragment.setArguments(getIntent().getExtras());
            
            // Add the fragment to the 'fragment_container' FrameLayout
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.fragment_container, firstFragment).commit();
        }
    }
    
    public void onGameSelected(Game game) {
		// //Game game = NDKBridge.queryRom(position);
        
        //LoadROMTask loadTask = new LoadROMTask(this);
        //loadTask.execute(new Integer(selectionRowID));
        //game.index= position;
        Intent i = new Intent();
        String title = "";//NDKBridge.getGameList(selectionRowID);
    	i.putExtra("com.neko68k.M1.title", title);
    	i.putExtra("com.neko68k.M1.position", game.index);
    	i.putExtra("com.neko68k.M1.game", game);
    	//startActivity(i);
    	setResult(RESULT_OK, i);                	
    	finish();  
		
	}
}
