/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.os.Build;
import android.os.Environment;
import android.os.PowerManager;
import android.os.Handler;
import android.os.Message;
import android.os.IBinder;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.util.Log;
import android.provider.Settings;

public class XCSoar extends Activity {
  private static final String TAG = "XCSoar";

  /**
   * Hack: this is set by onCreate(), to support the "testing"
   * package.
   */
  public static Class<?> serviceClass;

  private static NativeView nativeView;

  PowerManager.WakeLock wakeLock;

  BatteryReceiver batteryReceiver;

  /**
   * These are the flags initially set on our #Window.  Those flag
   * will be preserved by WindowUtil.leaveFullScreenMode().
   */
  int initialWindowFlags;

  boolean fullScreen = false;

  @Override protected void onCreate(Bundle savedInstanceState) {
    if (serviceClass == null)
      serviceClass = MyService.class;

    super.onCreate(savedInstanceState);

    Log.d(TAG, "ABI=" + Build.CPU_ABI);
    Log.d(TAG, "PRODUCT=" + Build.PRODUCT);
    Log.d(TAG, "MANUFACTURER=" + Build.MANUFACTURER);
    Log.d(TAG, "MODEL=" + Build.MODEL);
    Log.d(TAG, "DEVICE=" + Build.DEVICE);
    Log.d(TAG, "BOARD=" + Build.BOARD);
    Log.d(TAG, "FINGERPRINT=" + Build.FINGERPRINT);

    if (!Loader.loaded) {
      TextView tv = new TextView(this);
      tv.setText("Failed to load the native XCSoar libary.\n" +
                 "Report this problem to us, and include the following information:\n" +
                 "ABI=" + Build.CPU_ABI + "\n" +
                 "PRODUCT=" + Build.PRODUCT + "\n" +
                 "FINGERPRINT=" + Build.FINGERPRINT + "\n" +
                 "error=" + Loader.error);
      setContentView(tv);
      return;
    }

    NetUtil.initialise(this);

    IOIOHelper.onCreateContext(this);

    final Window window = getWindow();
    window.requestFeature(Window.FEATURE_NO_TITLE);

    TextView tv = new TextView(this);
    tv.setText("Loading XCSoar...");
    setContentView(tv);

    /* after setContentView(), Android has initialised a few default
       window flags, which we now remember for
       WindowUtil.leaveFullScreenMode() to avoid clearing those */
    initialWindowFlags = window.getAttributes().flags;

    batteryReceiver = new BatteryReceiver();
    registerReceiver(batteryReceiver,
                     new IntentFilter(Intent.ACTION_BATTERY_CHANGED));

    /* TODO: this sure is the wrong place to request permissions -
       we should request permissions when we need them, but
       implementing that is complicated, so for now, we do it
       here to give users a quick solution for the problem */
    requestAllPermissions();
  }

  private void quit() {
    nativeView = null;

    TextView tv = new TextView(XCSoar.this);
    tv.setText("Shutting down XCSoar...");
    setContentView(tv);

    finish();
  }

  final Handler quitHandler = new Handler() {
    public void handleMessage(Message msg) {
      quit();
    }
  };

  final Handler errorHandler = new Handler() {
    public void handleMessage(Message msg) {
      nativeView = null;
      TextView tv = new TextView(XCSoar.this);
      tv.setText(msg.obj.toString());
      setContentView(tv);
    }
  };

  private void acquireWakeLock() {
    final Window window = getWindow();
    window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    if (wakeLock != null)
      return;

    // Obtain an instance of the Android PowerManager class
    PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);

    // Create a WakeLock instance to keep the screen from timing out
    // Note: FULL_WAKE_LOCK is deprecated in favor of FLAG_KEEP_SCREEN_ON
    wakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK|
                              PowerManager.ACQUIRE_CAUSES_WAKEUP, TAG);

    // Activate the WakeLock
    wakeLock.acquire();
  }

  final Handler wakeLockHandler = new Handler() {
      public void handleMessage(Message msg) {
        acquireWakeLock();
      }
    };

  final Handler fullScreenHandler = new Handler() {
      public void handleMessage(Message msg) {
        fullScreen = msg.what != 0;
        applyFullScreen();
      }
    };

  private boolean isInMultiWindowModeCompat() {
    /* isInMultiWindowMode() was added in API 24 (Android 7.0) */
    return Build.VERSION.SDK_INT >= 24
      ? isInMultiWindowMode()
      : false;
  }

  boolean wantFullScreen() {
    return Loader.loaded && fullScreen && !isInMultiWindowModeCompat();
  }

  void applyFullScreen() {
    final Window window = getWindow();
    if (wantFullScreen())
      WindowUtil.enterFullScreenMode(window);
    else
      WindowUtil.leaveFullScreenMode(window, initialWindowFlags);
  }

  public void initNative() {
    if (!Loader.loaded)
      return;

    /* check if external storage is available; XCSoar doesn't work as
       long as external storage is being forwarded to a PC */
    String state = Environment.getExternalStorageState();
    if (!Environment.MEDIA_MOUNTED.equals(state)) {
      TextView tv = new TextView(this);
      tv.setText("External storage is not available (state='" + state
                 + "').  Please turn off USB storage.");
      setContentView(tv);
      return;
    }

    nativeView = new NativeView(this, quitHandler,
                                wakeLockHandler, fullScreenHandler,
                                errorHandler);
    setContentView(nativeView);
    // Receive keyboard events
    nativeView.setFocusableInTouchMode(true);
    nativeView.setFocusable(true);
    nativeView.requestFocus();
  }

  @Override protected void onPause() {
    if (nativeView != null)
      nativeView.onPause();
    super.onPause();
  }

  private void getHapticFeedbackSettings() {
    boolean hapticFeedbackEnabled;
    try {
      hapticFeedbackEnabled =
        (Settings.System.getInt(getContentResolver(),
                                Settings.System.HAPTIC_FEEDBACK_ENABLED)) != 0;
    } catch (Settings.SettingNotFoundException e) {
      hapticFeedbackEnabled = false;
    }

    if (nativeView != null)
      nativeView.setHapticFeedback(hapticFeedbackEnabled);
  }

  private static final String[] NEEDED_PERMISSIONS = new String[] {
    Manifest.permission.ACCESS_FINE_LOCATION,
    Manifest.permission.WRITE_EXTERNAL_STORAGE
  };

  private boolean hasAllPermissions() {
    for (String p : NEEDED_PERMISSIONS) {
      if (checkSelfPermission(p) != PackageManager.PERMISSION_GRANTED) {
        return false;
      }
    }

    return true;
  }

  private void requestAllPermissions() {
    if (android.os.Build.VERSION.SDK_INT < 23)
      /* we don't need to request permissions on this old Android
         version */
      return;

    /* starting with Android 6.0, we need to explicitly request all
       permissions before using them; mentioning them in the manifest
       is not enough */

    if (!hasAllPermissions()) {
      new AlertDialog.Builder(this)
        .setTitle("Location Permission")
        .setMessage("XCSoar needs to collect location data to enable live navigation calculation and IGC logger, even when the app is in the background")
        .setPositiveButton("Continue", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
              try {
                XCSoar.this.requestPermissions(NEEDED_PERMISSIONS, 0);
              } catch (IllegalArgumentException e) {
                Log.e(TAG, "could not request permissions: " + String.join(", ", NEEDED_PERMISSIONS), e);
              }
            }
        })
        .show();
    }
  }

  @Override protected void onResume() {
    super.onResume();

    if (!Loader.loaded)
      return;

    if (nativeView != null)
      nativeView.onResume();
    else
      initNative();
    getHapticFeedbackSettings();
  }

  @Override protected void onDestroy()
  {
    if (!Loader.loaded) {
      super.onDestroy();
      return;
    }

    if (batteryReceiver != null) {
      unregisterReceiver(batteryReceiver);
      batteryReceiver = null;
    }

    nativeView = null;

    // Release the WakeLock instance to re-enable screen timeouts
    if (wakeLock != null) {
      wakeLock.release();
      wakeLock = null;
    }

    IOIOHelper.onDestroyContext();

    super.onDestroy();
    System.exit(0);
  }

  @Override public boolean onKeyDown(int keyCode, final KeyEvent event) {
    // Overrides Back key to use in our app
    if (nativeView != null) {
      nativeView.onKeyDown(keyCode, event);
      return true;
    } else
      return super.onKeyDown(keyCode, event);
  }

  @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
    if (nativeView != null) {
      nativeView.onKeyUp(keyCode, event);
      return true;
    } else
      return super.onKeyUp(keyCode, event);
  }

  @Override public void onWindowFocusChanged(boolean hasFocus) {
    if (hasFocus && wantFullScreen())
      /* some Android don't restore immersive mode after returning to
         this app, so unfortunately we need to reapply those settings
         manually */
      WindowUtil.enableImmersiveMode(getWindow());

    super.onWindowFocusChanged(hasFocus);
  }

  @Override
  public void onMultiWindowModeChanged(boolean isInMultiWindowMode) {
    applyFullScreen();
  }

  @Override public boolean dispatchTouchEvent(final MotionEvent ev) {
    if (nativeView != null) {
      nativeView.onTouchEvent(ev);
      return true;
    } else
      return super.dispatchTouchEvent(ev);
  }
}
