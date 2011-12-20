// Please note this must be the package if you want to use XML-based preferences
package android.preference;
 

import android.content.Context;
import android.content.SharedPreferences;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TimePicker;

import com.neko68k.M1.NumberPicker;
import com.neko68k.emu.M1Android.R;
 
/**
 * A preference type that allows a user to choose a time
 */
public class TimePickerPreference extends DialogPreference  {
 
	/**
	 * The validation expression for this preference
	 */
	private static final String VALIDATION_EXPRESSION = "[0-2]*[0-9]:[0-5]*[0-9]";
 
	/**
	 * The default value for this preference
	 */
	private String defaultValue;
 
	/**
	 * @param context
	 * @param attrs
	 */
	public TimePickerPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
		initialize();
	}
 
	/**
	 * @param context
	 * @param attrs
	 * @param defStyle
	 */
	public TimePickerPreference(Context context, AttributeSet attrs,
			int defStyle) {
		super(context, attrs, defStyle);
		initialize();
	}
 
	/**
	 * Initialize this preference
	 */
	private void initialize() {
		this.setDialogLayoutResource(R.layout.timepicker);
		
		
		setPersistent(true);
	}
 
	
 
	
 
	/*
	 * (non-Javadoc)
	 * 
	 * @see android.preference.Preference#setDefaultValue(java.lang.Object)
	 */
	@Override
	public void setDefaultValue(Object defaultValue) {
		// BUG this method is never called if you use the 'android:defaultValue' attribute in your XML preference file, not sure why it isn't		
 
		super.setDefaultValue(defaultValue);
 
		if (!(defaultValue instanceof String)) {
			return;
		}
 
		if (!((String) defaultValue).matches(VALIDATION_EXPRESSION)) {
			return;
		}
 
		this.defaultValue = (String) defaultValue;
	}
	
	NumberPicker hours;
	NumberPicker minutes;
	NumberPicker seconds;
	
	@Override
	protected void onBindDialogView(View view) {

	    hours = (NumberPicker) view.findViewById(R.id.hours);
	    minutes = (NumberPicker) view.findViewById(R.id.minutes);
	    seconds = (NumberPicker) view.findViewById(R.id.seconds);
	    
	    SharedPreferences pref = getSharedPreferences();
	    
	    
	    hours.setValue((int) pref.getLong("defLenHours", 0)/120);
	    minutes.setValue((int) pref.getLong("defLenMins", 0)/60);
	    seconds.setValue((int) pref.getLong("defLenSecs", 0));
	    
	    if(minutes.getValue()==0){
	    	minutes.setValue(5);
	    }
	}
	
	@Override
	protected void onDialogClosed(boolean positiveResult){
		if(!positiveResult)
	        return;

	    SharedPreferences.Editor editor = getEditor();
	    //
	    editor.putLong("defLenHours",new Integer(hours.getValue()*120));
	    editor.putLong("defLenMins",new Integer(minutes.getValue()*60));
	    editor.putLong("defLenSecs",new Integer(seconds.getValue()));	   
	    editor.commit();

	    super.onDialogClosed(positiveResult);
	}
 
	
}