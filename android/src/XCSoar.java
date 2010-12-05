/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

package org.xcsoar;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.os.Build;

public class XCSoar extends Activity {
    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!Native.loaded) {
            TextView tv = new TextView(this);
            tv.setText("Failed to load the native XCSoar libary.\n" +
                       "Report this problem to us, and include the following information:\n" +
                       "ABI=" + Build.CPU_ABI + "\n" +
                       "PRODUCT=" + Build.PRODUCT + "\n" +
                       "FINGERPRINT=" + Build.FINGERPRINT + "\n" +
                       "error=" + Native.error);
            setContentView(tv);
            return;
        }

        // fullscreen mode
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        TextView tv = new TextView(this);
        tv.setText("Loading XCSoar...");
        setContentView(tv);
    }

    public void initSDL()
    {
        if (!Native.loaded)
            return;

        mGLView = new DemoGLSurfaceView(this);
        setContentView(mGLView);
        // Receive keyboard events
        mGLView.setFocusableInTouchMode(true);
        mGLView.setFocusable(true);
        mGLView.requestFocus();
    }

    @Override protected void onPause() {
        if( mGLView != null )
            mGLView.onPause();
        super.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        if( mGLView != null )
            mGLView.onResume();
        else
            initSDL();
    }

    @Override protected void onDestroy()
    {
        if( mGLView != null )
            mGLView.exitApp();
        super.onDestroy();
        System.exit(0);
    }

    @Override public boolean onKeyDown(int keyCode, final KeyEvent event) {
        // Overrides Back key to use in our app
        if( mGLView != null )
            mGLView.onKeyDown(keyCode, event);
        return true;
    }

    @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
        if( mGLView != null )
            mGLView.onKeyUp(keyCode, event);
        return true;
    }

    @Override public boolean dispatchTouchEvent(final MotionEvent ev) {
        if(mGLView != null)
            mGLView.onTouchEvent(ev);
        return true;
    }

    private static DemoGLSurfaceView mGLView = null;
}
