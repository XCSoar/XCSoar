/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
import android.app.PendingIntent;
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
import android.util.Log;
import android.provider.Settings;
import android.view.View;

public class XCSoar extends Activity {
  private static final String TAG = "XCSoar";

  /**
   * Hack: this is set by onCreate(), to support the "testing"
   * package.
   */
  protected static Class serviceClass;

  private static NativeView nativeView;

  PowerManager.WakeLock wakeLock;

  BatteryReceiver batteryReceiver;

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
    InternalGPS.Initialize();
    NonGPSSensors.Initialize();

    IOIOHelper.onCreateContext(this);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR)
      // Bluetooth suppoert was added in Android 2.0 "Eclair"
      BluetoothHelper.Initialize(this);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD)
      // the DownloadManager was added in Android 2.3 "Gingerbread"
      DownloadUtil.Initialise(this);

    // fullscreen mode
    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN|
                         WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    /* Workaround for layout problems in Android KitKat with immersive full
       screen mode: Sometimes the content view was not initialized with the
       correct size, which caused graphics artifacts. */
    if (android.os.Build.VERSION.SDK_INT >= 19) {
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN|
                           WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS|
                           WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR|
                           WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
    }

    enableImmersiveModeIfSupported();

    TextView tv = new TextView(this);
    tv.setText("Loading XCSoar...");
    setContentView(tv);

    batteryReceiver = new BatteryReceiver();
    registerReceiver(batteryReceiver,
                     new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
  }

  private void quit() {
    Log.d(TAG, "in quit()");

    nativeView = null;

    Log.d(TAG, "stopping service");
    stopService(new Intent(this, serviceClass));

    TextView tv = new TextView(XCSoar.this);
    tv.setText("Shutting down XCSoar...");
    setContentView(tv);

    Log.d(TAG, "finish()");
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

  public void initSDL() {
    if (!Loader.loaded)
      return;

    /* check if external storage is available; XCSoar doesn't work as
       long as external storage is being forwarded to a PC */
    String state = Environment.getExternalStorageState();
    Log.d(TAG, "getExternalStorageState() = " + state);
    if (!Environment.MEDIA_MOUNTED.equals(state)) {
      TextView tv = new TextView(this);
      tv.setText("External storage is not available (state='" + state
                 + "').  Please turn off USB storage.");
      setContentView(tv);
      return;
    }

    nativeView = new NativeView(this, quitHandler, errorHandler);
    setContentView(nativeView);
    // Receive keyboard events
    nativeView.setFocusableInTouchMode(true);
    nativeView.setFocusable(true);
    nativeView.requestFocus();

    // Obtain an instance of the Android PowerManager class
    PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);

    // Create a WakeLock instance to keep the screen from timing out
    // Note: FULL_WAKE_LOCK is deprecated in favor of FLAG_KEEP_SCREEN_ON
    wakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK|
                              PowerManager.ACQUIRE_CAUSES_WAKEUP, TAG);

    // Activate the WakeLock
    wakeLock.acquire();
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

  private void enableImmersiveModeIfSupported() {
    // Set / Reset the System UI visibility flags for Immersive Full Screen Mode, if supported
    if (android.os.Build.VERSION.SDK_INT >= 19)
      ImmersiveFullScreenMode.enable(getWindow().getDecorView());
  }

  @Override protected void onResume() {
    super.onResume();

    startService(new Intent(this, serviceClass));

    if (nativeView != null)
      nativeView.onResume();
    else
      initSDL();
    getHapticFeedbackSettings();
  }

  @Override protected void onDestroy()
  {
    Log.d(TAG, "in onDestroy()");

    if (batteryReceiver != null) {
      unregisterReceiver(batteryReceiver);
      batteryReceiver = null;
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD)
      DownloadUtil.Deinitialise(this);

    if (nativeView != null) {
      nativeView.exitApp();
      nativeView = null;
    }

    // Release the WakeLock instance to re-enable screen timeouts
    if (wakeLock != null) {
      wakeLock.release();
      wakeLock = null;
    }

    IOIOHelper.onDestroyContext();

    super.onDestroy();
    Log.d(TAG, "System.exit()");
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
    enableImmersiveModeIfSupported();
    super.onWindowFocusChanged(hasFocus);
  }

  @Override public boolean dispatchTouchEvent(final MotionEvent ev) {
    if (nativeView != null) {
      nativeView.onTouchEvent(ev);
      return true;
    } else
      return super.dispatchTouchEvent(ev);
  }
}
