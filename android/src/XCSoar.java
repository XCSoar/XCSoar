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
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.WindowMetrics;
import android.graphics.Insets;
import android.graphics.Rect;
import android.widget.TextView;
import android.os.Build;
import android.os.Environment;
import android.os.PowerManager;
import android.os.Handler;
import android.os.Looper;
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
  private PermissionHelper permissionHelper;

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

    mainHandler = new Handler(Looper.getMainLooper());

    /* Create PermissionHelper (no cloud dialog callback needed;
       cloud consent is handled during onboarding) */
    permissionHelper = new PermissionHelper(this, mainHandler, null);

    NativeView.initNative();

    NetUtil.initialise(this);

    IOIOHelper.onCreateContext(this);

    final Window window = getWindow();
    window.requestFeature(Window.FEATURE_NO_TITLE);

    /* Enable edge-to-edge display for SDK 30+. This is what EdgeToEdge.enable()
       does under the hood and is required for proper edge-to-edge support on
       Android 15+. The existing inset handling in applyFullScreen() handles
       the layout margins correctly. */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      window.setDecorFitsSystemWindows(false);
    }

    TextView tv = new TextView(this);
    tv.setText("Loading XCSoar...");
    setContentView(tv);

    /* after setContentView(), Android has initialised a few default
       window flags, which we now remember for
       WindowUtil.leaveFullScreenMode() to avoid clearing those */
    initialWindowFlags = window.getAttributes().flags;

    /* Apply fullscreen mode early (with default value = true) to avoid
       visible layout changes when the native code loads the profile.
       The default fullscreen setting is true (DisplaySettings::SetDefaults()).
       If the user has disabled fullscreen in their profile, it will be
       updated later by native code via setFullScreen(). */
    fullScreen = true;
    applyFullScreen();

    submitConfiguration(getResources().getConfiguration());

    batteryReceiver = new BatteryReceiver();
    BroadcastUtil.registerReceiver(this, batteryReceiver,
                                   new IntentFilter(Intent.ACTION_BATTERY_CHANGED));

    /* POST_NOTIFICATIONS permission will be requested through the consent dialog
       flow when needed (e.g., when the foreground service needs to start).
       This ensures the consent rationale is always shown before requesting permission. */
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
        /* Called by native code when the fullscreen setting is loaded from
           the profile. This may override the initial default value that was
           applied in onCreate(). */
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
    
    if (nativeView != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      final View decorView = window.getDecorView();
      decorView.post(new Runnable() {
        @Override
        public void run() {
          if (nativeView == null)
            return;

          boolean is_fullscreen = wantFullScreen();
          android.view.ViewGroup.LayoutParams layoutParams = nativeView.getLayoutParams();
          
          if (layoutParams instanceof android.view.ViewGroup.MarginLayoutParams) {
            android.view.ViewGroup.MarginLayoutParams marginParams =
              (android.view.ViewGroup.MarginLayoutParams) layoutParams;
            
            if (is_fullscreen) {
              marginParams.setMargins(0, 0, 0, 0);
            } else {
              WindowMetrics windowMetrics = getWindowManager().getCurrentWindowMetrics();
              final WindowInsets windowInsets = windowMetrics.getWindowInsets();
              Insets insets = windowInsets.getInsets(
                WindowInsets.Type.statusBars() |
                WindowInsets.Type.navigationBars() |
                WindowInsets.Type.displayCutout());
              marginParams.setMargins(insets.left, insets.top, insets.right, insets.bottom);
            }
            nativeView.setLayoutParams(marginParams);
          }
        }
      });
    }
    
    if (nativeView != null && Loader.loaded) {
      final View decorView = window.getDecorView();
      decorView.post(new Runnable() {
        @Override
        public void run() {
          decorView.post(new Runnable() {
            @Override
            public void run() {
              int display_width = nativeView.getWidth();
              int display_height = nativeView.getHeight();
              int inset_left = 0, inset_top = 0, inset_right = 0, inset_bottom = 0;
              
              if (display_width > 0 && display_height > 0) {
                nativeView.resizedNative(display_width, display_height, inset_left, inset_top, inset_right, inset_bottom);
              }
            }
          });
        }
      });
    }
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

    /* Resume processing permission queue if there are pending requests */
    if (permissionHelper != null)
      permissionHelper.resumePermissionProcessing();
  }

  @Override protected void onDestroy()
  {
    if (!Loader.loaded) {
      super.onDestroy();
      return;
    }

    /* Mark the app as shutting down so that MyService will not restart
       itself after System.exit() kills the process.  The flag must be
       written synchronously (commit(), not apply()) because
       System.exit() follows shortly. */
    getApplicationContext()
      .getSharedPreferences("xcsoar_service", Context.MODE_PRIVATE)
      .edit()
      .putBoolean("app_shutdown", true)
      .commit();

    try {
      stopService(new Intent(this, org.xcsoar.MyService.class));
    } catch (Exception e) {
      /* Ignore exceptions when stopping service during app shutdown.
         The service will be cleaned up by the system if needed. */
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
    if (nativeView != null && nativeView.onKeyDown(keyCode, event))
      return true;

    return super.onKeyDown(keyCode, event);
  }

  @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
    if (nativeView != null && nativeView.onKeyUp(keyCode, event))
      return true;

    return super.onKeyUp(keyCode, event);
  }

  @Override public void onWindowFocusChanged(boolean hasFocus) {
    if (hasFocus && wantFullScreen()) {
      /* some Android don't restore fullscreen settings after returning to
         this app or after orientation changes, so we need to reapply all
         fullscreen settings (immersive mode + display cutout mode) manually */
      WindowUtil.enterFullScreenMode(getWindow());
    }

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

    /* Reapply fullscreen settings after orientation change.
       The display cutout mode and window layout parameters can be reset
       during orientation changes, so we need to reapply them. */
    applyFullScreen();
    
    /* applyFullScreen() will handle updating the native view with the correct size.
       No need to duplicate the logic here - applyFullScreen() already posts to decorView
       to ensure layout is complete before getting the SurfaceView size. */
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String[] permissions,
                                                      int[] grantResults) {
    if (permissionHelper != null) {
      permissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }
  }

  /* virtual methods from PermissionManager */

  @Override
  public boolean requestPermission(String permission, PermissionHandler handler) {
    if (permissionHelper != null)
      return permissionHelper.requestPermission(permission, handler);
    return false;
  }

  @Override
  public synchronized void cancelRequestPermission(PermissionHandler handler) {
    if (permissionHelper != null)
      permissionHelper.cancelRequestPermission(handler);
  }

  @Override
  public boolean areLocationPermissionsGranted() {
    if (permissionHelper != null)
      return permissionHelper.areLocationPermissionsGranted();
    return false;
  }

  @Override
  public boolean isNotificationPermissionGranted() {
    if (permissionHelper != null)
      return permissionHelper.isNotificationPermissionGranted();
    return true;
  }

  @Override
  public void requestAllLocationPermissionsDirect() {
    if (permissionHelper != null)
      permissionHelper.requestAllLocationPermissionsDirect();
  }

  @Override
  public void requestNotificationPermissionDirect() {
    if (permissionHelper != null)
      permissionHelper.requestNotificationPermissionDirect();
  }

  @Override
  public void suppressPermissionDialogs() {
    if (permissionHelper != null)
      permissionHelper.suppressPermissionDialogs();
  }

  @Override
  public void onDisclosureResult(boolean accepted) {
    if (permissionHelper != null)
      permissionHelper.onDisclosureResult(accepted);
  }
}
