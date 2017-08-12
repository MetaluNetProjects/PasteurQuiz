package net.metalu.PasteurQuiz;

import android.content.Intent;
import android.os.Bundle;
import android.content.Context;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.MotionEvent;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import cc.openframeworks.OFAndroid;
import android.view.WindowManager;
import android.graphics.PixelFormat;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class OFActivity extends cc.openframeworks.OFActivity{

	OFAndroid ofApp;

	@Override
    public void onCreate(Bundle savedInstanceState)
    { 
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);

        // every time someone enters the kiosk mode, set the flag true
        PrefUtils.setKioskModeActive(true/*false*/, getApplicationContext());
		preventStatusBarExpansion(this);
		
        String packageName = getPackageName();

        ofApp = new OFAndroid(packageName,this);
    }
	
	@Override
	public void onDetachedFromWindow() {
	}
	
    @Override
    protected void onPause() {
        super.onPause();
        ofApp.pause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        ofApp.resume();
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
	if (OFAndroid.keyDown(keyCode, event)) {
	    return true;
	} else {
	    return super.onKeyDown(keyCode, event);
	}
    }
    
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
	if (OFAndroid.keyUp(keyCode, event)) {
	    return true;
	} else {
	    return super.onKeyUp(keyCode, event);
	}
    }

	/*@Override
	public void onBackPressed() {
		// nothing to do here
		// … really
	}*/

    
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if(!hasFocus) {
            // Close every kind of system dialog
            Intent closeDialog = new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
            sendBroadcast(closeDialog);
        }
    }
	
	
    // Menus
    // http://developer.android.com/guide/topics/ui/menus.html
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	// Create settings menu options from here, one by one or infalting an xml
        return super.onCreateOptionsMenu(menu);
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	// This passes the menu option string to OF
    	// you can add additional behavior from java modifying this method
    	// but keep the call to OFAndroid so OF is notified of menu events
    	if(OFAndroid.menuItemSelected(item.getItemId())){
    		
    		return true;
    	}
    	return super.onOptionsItemSelected(item);
    }
    

    @Override
    public boolean onPrepareOptionsMenu (Menu menu){
    	// This method is called every time the menu is opened
    	//  you can add or remove menu options from here
    	return  super.onPrepareOptionsMenu(menu);
    }


//	comm with OF

    public void quit(){
		PrefUtils.setKioskModeActive(false, getApplicationContext());        
		finish();
		//OFAndroid.exit();
		//ofApp.pause();
		//ofApp.stop();
		System.exit(0);
		
		OFAndroid.exit();
		finish();
		System.exit(0);
	}

    public void systemMessage(Object... args) {
		if((args.length>0) && args[0].getClass().equals(String.class) && args[0].equals("quit")) {
			quit();
		}
		else if((args.length>1) && args[0].getClass().equals(String.class) && args[0].equals("setPref")) {
			//setPref(java.util.Arrays.copyOfRange(args, 1, args.length));
		}
	}
	
	public void receiveMessage(final Object... args) {
		this.runOnUiThread(new Runnable(){
			@Override
			public void run() {
				if((args.length>1) && args[0].getClass().equals(String.class) && args[0].equals("toSYSTEM")) {
					systemMessage(java.util.Arrays.copyOfRange(args, 1 , args.length));
				}
			}
		});
	}	

    public static native void sendToPd(Object...args);
    
    public void sendToPdAsync(final Object...args) {
    	this.runOnUiThread(new Runnable(){
			@Override
			public void run() {
				sendToPd(args);
			}
		});
    }

	public static void preventStatusBarExpansion(Context context) {
		WindowManager manager = ((WindowManager) context.getApplicationContext().getSystemService(Context.WINDOW_SERVICE));

		WindowManager.LayoutParams localLayoutParams = new WindowManager.LayoutParams();
		localLayoutParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_ERROR;
		localLayoutParams.gravity = Gravity.TOP;
		localLayoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE|WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL|WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;

		localLayoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;

		int resId = context.getResources().getIdentifier("status_bar_height", "dimen", "android");
		int result = 0;
		if (resId > 0) {
		  result = context.getResources().getDimensionPixelSize(resId);
		} else {
		  // Use Fallback size:
		  result = 60; // 60px Fallback
		}

		localLayoutParams.height = result;
		localLayoutParams.format = PixelFormat.TRANSPARENT;

		customViewGroup view = new customViewGroup(context);
		manager.addView(view, localLayoutParams);
	}

	public static class customViewGroup extends ViewGroup {
		public customViewGroup(Context context) {
		    super(context);
		}

		@Override
		protected void onLayout(boolean changed, int l, int t, int r, int b) {
		}

		@Override
		public boolean onInterceptTouchEvent(MotionEvent ev) {
		    // Intercepted touch!
		    return true;
		}
	}
}



