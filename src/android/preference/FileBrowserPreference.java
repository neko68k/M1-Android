package android.preference;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;

public class FileBrowserPreference extends Preference {

	public FileBrowserPreference(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
	}

	public FileBrowserPreference(Context context, AttributeSet attrs) {
		  super(context, attrs);
		 }
		 
	public FileBrowserPreference(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}
	
	@Override
	protected View onCreateView(ViewGroup parent){
		RelativeLayout layout = new RelativeLayout(getContext());
		
		return layout;
	}
	 
	/*setOnPreferenceClickListener(
	   new Preference.OnPreferenceChangeListener() {
        public void onClick(View v) {
        	
        }

		public boolean onPreferenceChange(Preference preference, Object newValue) {
			// TODO Auto-generated method stub
			return false;
		}
    });*/
}
