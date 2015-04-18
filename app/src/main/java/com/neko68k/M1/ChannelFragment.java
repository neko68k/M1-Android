package com.neko68k.M1;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

/**
 * Created by neko on 4/18/15.
 */
public class ChannelFragment extends ListFragment {
    @Override
    public void onPause() {

        super.onPause();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setHasOptionsMenu(true);

    }

    /*@Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        inflater.inflate(R.menu.gamelistmenu, menu);
    }*/

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        //SQLiteDatabase db = NDKBridge.m1db.getReadableDatabase();
        //Cursor c = GameListOpenHelper.getAllTitles(db);
        Context cxt = getActivity();
        /*GameListCursorAdapter adapter = new GameListCursorAdapter(cxt,
                R.layout.gamelist_detailed,
                c, new String[] { "title",
                "year", "mfg", "sys", "soundhw" }, new int[] { R.id.title,
                R.id.year, R.id.mfg, R.id.board, R.id.hardware }, GameListActivity.getSortType());
*/
        final ListView lv = getListView();
        lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> av, View v, int pos, long id) {
                onListItemClick(lv, v, pos, id);
            }
        });


        //this.setListAdapter(adapter);
        //adapter.notifyDataSetChanged();
        //lv.setFastScrollEnabled(true);
        //db.close();

    }
    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        // maybe do nothing?

    }

    /*OnItemSelectedListener mCallback;

    // Container Activity must implement this interface
    public interface OnItemSelectedListener {
        //public void onGameSelected(Game game);
    }



    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        // This makes sure that the container activity has implemented
        // the callback interface. If not, it throws an exception
        try {
            mCallback = (OnItemSelectedListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement OnItemSelectedListener");
        }
    }*/
}
