// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Map;
import java.util.TreeMap;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Html;
import android.text.method.LinkMovementMethod;
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
import android.content.res.Configuration;
import android.util.Log;
import android.provider.Settings;

public class XCSoar extends Activity implements PermissionManager {
  private static final String TAG = "XCSoar";

  private static NativeView nativeView;

  private Handler mainHandler;

  PowerManager.WakeLock wakeLock;

  BatteryReceiver batteryReceiver;

  /**
   * These are the flags initially set on our #Window.  Those flag
   * will be preserved by WindowUtil.leaveFullScreenMode().
   */
  int initialWindowFlags;

  boolean fullScreen = false;

  @Override protected void onCreate(Bundle savedInstanceState) {
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

    mainHandler = new Handler(getMainLooper());

    NativeView.initNative();

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

    submitConfiguration(getResources().getConfiguration());

    batteryReceiver = new BatteryReceiver();
    registerReceiver(batteryReceiver,
                     new IntentFilter(Intent.ACTION_BATTERY_CHANGED));

    /* WRITE_EXTERNAL_STORAGE has no effect on Build.VERSION_CODES.R
       (Android 11 or newer); we request it on older versions so users
       can keep using /sdcard/XCSoarData */
    if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
      requestPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, null);
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
                                errorHandler,
                                this);
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

    NativeView.deinitNative();

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

  private void submitConfiguration(Configuration config) {
    final boolean nightMode = (config.uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
    NativeView.onConfigurationChangedNative(nightMode);
  }

  @Override public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    submitConfiguration(newConfig);
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String[] permissions,
                                                      int[] grantResults) {
    PermissionHandler handler = permissionHandlers.remove(requestCode);
    if (handler != null)
      // grantResults is empty when user cancels
      handler.onRequestPermissionsResult(grantResults.length > 0 &&
                                         grantResults[0] == PackageManager.PERMISSION_GRANTED);
  }

  private static String getPermissionRationale(String permission) {
    if (permission == Manifest.permission.ACCESS_FINE_LOCATION)
      return "XCSoar needs permission to access your GPS location - obviously, because XCSoar's purpose is to help you navigate an aircraft.";
    else if (permission == Manifest.permission.ACCESS_BACKGROUND_LOCATION)
      return "Several optional features (e.g. flight logging and score calculation) benefit from location access while XCSoar is in background. " +
        "If you choose not to allow this, calculation results may be incomplete.";
    else if (permission == Manifest.permission.BLUETOOTH_CONNECT ||
             permission == Manifest.permission.BLUETOOTH_SCAN)
      return "If you want XCSoar to connect to Bluetooth sensors, it needs your permission.";
    else
      return null;
  }

  private void showRequestPermissionRationale(final String permission,
                                              final String rationale,
                                              final PermissionHandler handler) {
    /* using HTML so the privacy policy link is clickable */
    final String html = "<p>" +
      "XCSoar is free software developed by volunteers just for fun. " +
      "The project is non-profit - you don't pay for XCSoar, and we don't sell your data (or anything else). " +
      "</p>" +
      "<p><big>" +
      rationale +
      "</big></p>" +
      "<p>" +
      "All those accesses are only in your own interest; we don't collect your data and we don't track you (unless you explicitly ask XCSoar to). " +
      "</p>" +
      "<p>" +
      "More details can be found in the <a href=\"https://github.com/XCSoar/XCSoar/blob/master/PRIVACY.md\">Privacy policy</a>. " +
      "</p>";

    final TextView tv  = new TextView(this);
    tv.setMovementMethod(LinkMovementMethod.getInstance());
    tv.setText(Html.fromHtml(html));

    new AlertDialog.Builder(this)
      .setTitle("Requesting your permission")
      .setView(tv)
      .setPositiveButton("OK", new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
            doRequestPermission(permission, handler);
          }
        })
      .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
            if (handler != null)
              handler.onRequestPermissionsResult(false);
          }
        })
      .setOnCancelListener(new DialogInterface.OnCancelListener() {
          @Override
          public void onCancel(DialogInterface dialog) {
            if (handler != null)
              handler.onRequestPermissionsResult(false);
          }
        })
      .setOnDismissListener(new DialogInterface.OnDismissListener() {
          @Override
          public void onDismiss(DialogInterface dialog) {
            if (handler != null)
              handler.onRequestPermissionsResult(false);
          }
        })
      .show();
  }

  /**
   * @return true if an alert is being displayed (and an asynchronous
   * callback will then actually request the permission), false if no
   * rationale was displayed (and no permission was requested)
   */
  private boolean showRequestPermissionRationaleIndirect(final String permission,
                                                         final PermissionHandler handler) {
    final String rationale = getPermissionRationale(permission);
    if (rationale == null)
      return false;

    mainHandler.post(new Runnable() {
        @Override public void run() {
          showRequestPermissionRationale(permission, rationale, handler);
        }
      });

    return true;
  }

  private synchronized int addPermissionHandler(PermissionHandler handler) {
    final int id = nextPermissionHandlerId++;

    if (handler != null)
      permissionHandlers.put(id, handler);

    return id;
  }

  private void doRequestPermission(String permission,
                                   PermissionHandler handler) {
    requestPermissions(new String[]{permission},
                       addPermissionHandler(handler));
  }

  /* virtual methods from PermissionManager */

  private final Map<Integer, PermissionHandler> permissionHandlers =
    new TreeMap<Integer, PermissionHandler>();
  private int nextPermissionHandlerId = 0;

  @Override
  public boolean requestPermission(String permission, PermissionHandler handler) {
    if (android.os.Build.VERSION.SDK_INT < 23)
      /* we don't need to request permissions on this old Android
         version */
      return true;

    if (checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED)
      /* we already have the permission */
      return true;

    if (shouldShowRequestPermissionRationale(permission) &&
        showRequestPermissionRationaleIndirect(permission, handler))
      return false;

    doRequestPermission(permission, handler);
    return false;
  }

  @Override
  public synchronized void cancelRequestPermission(PermissionHandler handler) {
    permissionHandlers.values().remove(handler);
  }
}
