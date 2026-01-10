// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Map;
import java.util.TreeMap;
import java.util.Queue;
import java.util.LinkedList;

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
import android.content.pm.PermissionInfo;
import android.content.res.Configuration;
import android.util.Log;
import android.provider.Settings;
import android.app.NotificationManager;

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

    mainHandler = new Handler(Looper.getMainLooper());

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

    /* WRITE_EXTERNAL_STORAGE has no effect on Build.VERSION_CODES.R
       (Android 11 or newer); we request it on older versions so users
       can keep using /sdcard/XCSoarData */
    if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
      requestPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, null);

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
    if (wantFullScreen()) {
      WindowUtil.enterFullScreenMode(window);
      updateSystemGestureInsets();
    } else {
      WindowUtil.leaveFullScreenMode(window, initialWindowFlags);
      topGestureInset = 0;
      bottomGestureInset = 0;
      gestureInsetsInitialized = false;
    }
    
    if (nativeView != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      final View decorView = window.getDecorView();
      decorView.post(new Runnable() {
        @Override
        public void run() {
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
    processNextPermission();
  }

  @Override protected void onDestroy()
  {
    if (!Loader.loaded) {
      super.onDestroy();
      return;
    }

    /* Stop the foreground service when app is being destroyed.
       This is the only place where we stop the service - it continues running
       when the app goes to background to ensure continuous IGC logging and
       safety warnings. */
    if (nativeView != null) {
      Context context = nativeView.getContext();
      if (context != null) {
        try {
          context.stopService(new Intent(context, org.xcsoar.MyService.class));
        } catch (Exception e) {
          /* Ignore exceptions when stopping service during app shutdown.
             The service will be cleaned up by the system if needed. */
        }
      }
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
    if (hasFocus && wantFullScreen()) {
      /* some Android don't restore fullscreen settings after returning to
         this app or after orientation changes, so we need to reapply all
         fullscreen settings (immersive mode + display cutout mode) manually */
      WindowUtil.enterFullScreenMode(getWindow());
      /* Update gesture insets when regaining focus in fullscreen mode */
      updateSystemGestureInsets();
    }

    super.onWindowFocusChanged(hasFocus);
  }

  @Override
  public void onMultiWindowModeChanged(boolean isInMultiWindowMode) {
    applyFullScreen();
  }

  private float initialTouchY = -1;
  private boolean isTopEdgeGesture = false;
  private boolean isBottomEdgeGesture = false;
  private int topGestureInset = 0;
  private int bottomGestureInset = 0;
  private boolean gestureInsetsInitialized = false;
  private int cachedWindowHeight = -1;

  /**
   * Update system gesture insets. Should be called when entering fullscreen mode.
   */
  private void updateSystemGestureInsets() {
    if (Build.VERSION.SDK_INT >= 29) {
      final View decorView = getWindow().getDecorView();
      final WindowInsets rootInsets = decorView.getRootWindowInsets();
      if (rootInsets != null) {
        final Insets gestureInsets = rootInsets.getSystemGestureInsets();
        topGestureInset = gestureInsets.top;
        bottomGestureInset = gestureInsets.bottom;
        gestureInsetsInitialized = true;
      } else {
        /* Fallback values if insets not available yet */
        topGestureInset = 30;
        bottomGestureInset = 30;
        gestureInsetsInitialized = false;
      }
    } else {
      gestureInsetsInitialized = false;
    }
  }

  private boolean isInTopGestureArea(float y, int topInset) {
    return topInset > 0 && y <= topInset;
  }

  private boolean isInBottomGestureArea(float y, int windowHeight, int bottomInset) {
    return windowHeight > 0 && bottomInset > 0 && y >= (windowHeight - bottomInset);
  }

  /**
   * Check if movement is in the system gesture direction.
   * Requires minimum movement threshold to avoid blocking taps.
   */
  private boolean isSystemGestureDirection(float initialY, float currentY, boolean isTopEdge) {
    final float dy = currentY - initialY;
    final float absDy = Math.abs(dy);
    
    final float swipeThreshold = 10.0f;
    if (absDy < swipeThreshold)
      return false;
    
    if (isTopEdge)
      return dy > 0;
    else
      return dy < 0;
  }

  private void resetGestureState() {
    initialTouchY = -1;
    isTopEdgeGesture = false;
    isBottomEdgeGesture = false;
    cachedWindowHeight = -1;
  }

  /**
   * Don't block touches in ACTION_DOWN - just mark potential gesture areas.
   * This allows taps on buttons to work normally. Only swipes will be blocked in ACTION_MOVE.
   */
  private boolean handleGestureActionDown(MotionEvent ev, int windowHeight) {
    final float y = ev.getY();
    
    cachedWindowHeight = windowHeight;
    
    if (isInTopGestureArea(y, topGestureInset)) {
      initialTouchY = y;
      isTopEdgeGesture = true;
      isBottomEdgeGesture = false;
      return false;
    }
    
    if (isInBottomGestureArea(y, windowHeight, bottomGestureInset)) {
      initialTouchY = y;
      isTopEdgeGesture = false;
      isBottomEdgeGesture = true;
      return false;
    }
    
    resetGestureState();
    return false;
  }

  /**
   * Only block if there's a clear swipe in the system gesture direction:
   * - From top: downward swipe (top to bottom)
   * - From bottom: upward swipe (bottom to top)
   */
  private boolean handleGestureActionMove(MotionEvent ev, int windowHeight) {
    final float y = ev.getY();
    
    final int effectiveHeight = cachedWindowHeight > 0 ? cachedWindowHeight : windowHeight;
    
    if (isTopEdgeGesture) {
      if (isSystemGestureDirection(initialTouchY, y, true))
        return true;
      resetGestureState();
      return false;
    }
    
    if (isBottomEdgeGesture) {
      if (isSystemGestureDirection(initialTouchY, y, false))
        return true;
      resetGestureState();
      return false;
    }
    
    return false;
  }

  @Override public boolean dispatchTouchEvent(final MotionEvent ev) {
    /* On API 29+, detect swipes from system gesture areas and let the system
       handle them. On older Android, immersive mode handles this automatically. */
    if (wantFullScreen() && nativeView != null && Build.VERSION.SDK_INT >= 29) {
      final int action = ev.getActionMasked();
      final int windowHeight = getWindow().getDecorView().getHeight();

      switch (action) {
      case MotionEvent.ACTION_DOWN:
        handleGestureActionDown(ev, windowHeight);
        break;

      case MotionEvent.ACTION_MOVE:
        if (handleGestureActionMove(ev, windowHeight))
          return false;
        break;

      case MotionEvent.ACTION_UP:
      case MotionEvent.ACTION_CANCEL:
        resetGestureState();
        break;
      }
    }

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
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String[] permissions,
                                                      int[] grantResults) {
    PermissionHandler handler = permissionHandlers.remove(requestCode);
    if (handler != null) {
      // grantResults is empty when user cancels
      // For multiple permissions, all must be granted
      boolean granted = grantResults.length > 0;
      for (int i = 0; i < grantResults.length; i++) {
        if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
          granted = false;
          break;
        }
      }

      // If permission was denied and we can't show rationale, user selected "Don't ask again"
      if (!granted && grantResults.length > 0 && 
          grantResults[0] == PackageManager.PERMISSION_DENIED &&
          permissions.length > 0 &&
          !shouldShowRequestPermissionRationale(permissions[0])) {
        // Show a message directing user to settings
        final String permissionLabel = getPermissionLabel(permissions[0]);
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
              android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(XCSoar.this);
              builder.setTitle("Permission Required");
              builder.setMessage("XCSoar needs " + permissionLabel + " permission, but it was previously denied. " +
                               "Please grant it in Settings > Apps > XCSoar > Permissions.");
              builder.setPositiveButton("Open Settings", new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    intent.setData(android.net.Uri.parse("package:" + getPackageName()));
                    startActivity(intent);
                  }
                });
              builder.setNegativeButton("Cancel", null);
              builder.show();
            }
          });
      }

      handler.onRequestPermissionsResult(granted);
    }
  }

  private static String getBackgroundLocationRationale() {
    return "XCSoar needs permission to access your location in the background (when the app is closed) for flight logging and score calculation. " +
      "If you choose not to allow background location, calculation results may be incomplete.";
  }

  private static String getPermissionRationale(String permission) {
    if (isForegroundLocationPermission(permission))
      return "XCSoar needs permission to access your GPS location - obviously, because XCSoar's purpose is to help you navigate an aircraft.";
    else if (isBackgroundLocationPermission(permission))
      return getBackgroundLocationRationale();
    else if (isBluetoothPermission(permission))
      return "If you want XCSoar to connect to Bluetooth sensors, it needs your permission.";
    else if (Manifest.permission.POST_NOTIFICATIONS.equals(permission))
      return "XCSoar needs permission to show notifications. A notification is required by Android for background operation, and provides a quick way to return to the app. Background operation is essential for continuous flight logging and safety warnings.";
    else
      return null;
  }

  /**
   * Get a human-friendly label for a permission string.
   * Attempts to use PackageManager to get a localized label,
   * falling back to a predefined map or simplified string.
   */
  private String getPermissionLabel(String permission) {
    PackageManager packageManager = getPackageManager();
    try {
      PermissionInfo permissionInfo = packageManager.getPermissionInfo(permission, 0);
      CharSequence label = permissionInfo.loadLabel(packageManager);
      if (label != null && label.length() > 0)
        return label.toString();
    } catch (PackageManager.NameNotFoundException e) {
      /* Fall through to fallback map */
    }

    /* Fallback map for common permissions */
    if (isForegroundLocationPermission(permission))
      return "Location";
    else if (isBackgroundLocationPermission(permission))
      return "Background Location";
    else if (Manifest.permission.BLUETOOTH_CONNECT.equals(permission))
      return "Bluetooth Connect";
    else if (Manifest.permission.BLUETOOTH_SCAN.equals(permission))
      return "Bluetooth Scan";
    else if (Manifest.permission.WRITE_EXTERNAL_STORAGE.equals(permission))
      return "Storage";
    else if (Manifest.permission.POST_NOTIFICATIONS.equals(permission))
      return "Notifications";
    else {
      /* Simplified fallback: extract last component of permission name */
      int lastDot = permission.lastIndexOf('.');
      if (lastDot >= 0 && lastDot < permission.length() - 1)
        return permission.substring(lastDot + 1).replace('_', ' ');
      return permission;
    }
  }

  /**
   * Check if a permission is a location permission (foreground or background).
   */
  private static boolean isLocationPermission(String permission) {
    return Manifest.permission.ACCESS_FINE_LOCATION.equals(permission) ||
           Manifest.permission.ACCESS_BACKGROUND_LOCATION.equals(permission);
  }

  /**
   * Check if a permission is foreground location permission.
   */
  private static boolean isForegroundLocationPermission(String permission) {
    return Manifest.permission.ACCESS_FINE_LOCATION.equals(permission);
  }

  /**
   * Check if a permission is background location permission.
   */
  private static boolean isBackgroundLocationPermission(String permission) {
    return Manifest.permission.ACCESS_BACKGROUND_LOCATION.equals(permission);
  }

  /**
   * Check if a permission is a Bluetooth permission (connect or scan).
   */
  private static boolean isBluetoothPermission(String permission) {
    return Manifest.permission.BLUETOOTH_CONNECT.equals(permission) ||
           Manifest.permission.BLUETOOTH_SCAN.equals(permission);
  }

  /**
   * Check if a permission is notification permission (Android 13+).
   */
  private static boolean isNotificationPermission(String permission) {
    return Manifest.permission.POST_NOTIFICATIONS.equals(permission);
  }


  /**
   * Check if notifications are enabled for this app, and prompt user to enable
   * them if disabled. This should be called after POST_NOTIFICATIONS permission
   * is granted on Android 13+.
   */
  private void checkNotificationEnablement() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      NotificationManager notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
      if (notificationManager != null && !notificationManager.areNotificationsEnabled()) {
        /* Notifications are disabled at app level. Show dialog directing user to settings. */
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
              android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(XCSoar.this);
              builder.setTitle("Notifications Disabled");
              builder.setMessage("XCSoar needs notifications enabled to run in the background for continuous flight logging and safety warnings. Please enable notifications in system settings.");
              builder.setPositiveButton("Open Settings", new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                      Intent intent = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS);
                      intent.putExtra(Settings.EXTRA_APP_PACKAGE, getPackageName());
                      startActivity(intent);
                    } else {
                      /* On Android < 8.0, open general app settings */
                      Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                      intent.setData(android.net.Uri.parse("package:" + getPackageName()));
                      startActivity(intent);
                    }
                  }
                });
              builder.setNegativeButton("Cancel", null);
              builder.show();
            }
          });
      }
    }
  }

  /**
   * Build HTML template for permission rationale dialog.
   * Uses HTML so the privacy policy link is clickable.
   */
  private static String buildPermissionRationaleHtml(String rationale) {
    return "<p>" +
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
  }

  /**
   * Request permission after user accepts the rationale dialog.
   * Handles both Bluetooth permissions case, single permission case, and background location follow-up.
   */
  private void requestPermissionAfterDialog(String permission, PermissionHandler handler,
                                             boolean requestBothBluetooth, boolean requestBackgroundAfter,
                                             boolean isBluetooth) {
    if (requestBothBluetooth) {
      final String[] bluetoothPermissions = {
        Manifest.permission.BLUETOOTH_SCAN,
        Manifest.permission.BLUETOOTH_CONNECT
      };
      doRequestPermissions(bluetoothPermissions, new PermissionHandler() {
          @Override
          public void onRequestPermissionsResult(boolean granted) {
            pendingBluetoothPermissions.remove(Manifest.permission.BLUETOOTH_SCAN);
            pendingBluetoothPermissions.remove(Manifest.permission.BLUETOOTH_CONNECT);
            isProcessingPermission = false;
            if (handler != null)
              handler.onRequestPermissionsResult(granted);
            processNextPermission();
          }
        });
    } else {
      if (isBluetooth)
        pendingBluetoothPermissions.add(permission);
      doRequestPermission(permission, new PermissionHandler() {
          @Override
          public void onRequestPermissionsResult(boolean granted) {
            if (isBluetooth)
              pendingBluetoothPermissions.remove(permission);
            if (handler != null)
              handler.onRequestPermissionsResult(granted);

            if (granted && requestBackgroundAfter &&
                android.os.Build.VERSION.SDK_INT >= 29) {
              final String bgPermission = Manifest.permission.ACCESS_BACKGROUND_LOCATION;
              if (checkSelfPermission(bgPermission) != PackageManager.PERMISSION_GRANTED) {
                mainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                      requestPermission(bgPermission, null);
                    }
                  });
              }
            }

            /* After POST_NOTIFICATIONS is granted, check if notifications are enabled
               at the app level and prompt user to enable them if disabled. */
            if (granted && isNotificationPermission(permission) &&
                android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
              mainHandler.post(new Runnable() {
                  @Override
                  public void run() {
                    checkNotificationEnablement();
                  }
                });
            }
          }
        });
    }
  }

  private void showRequestPermissionRationale(final String permission,
                                              final String rationale,
                                              final PermissionHandler handler) {
    showRequestPermissionRationale(permission, rationale, handler, false, false);
  }

  private void showRequestPermissionRationale(final String permission,
                                              final String rationale,
                                              final PermissionHandler handler,
                                              final boolean requestBackgroundAfter,
                                              final boolean requestBothBluetooth) {
    final String html = buildPermissionRationaleHtml(rationale);

    final TextView tv  = new TextView(this);
    tv.setMovementMethod(LinkMovementMethod.getInstance());
    tv.setText(Html.fromHtml(html));

    final boolean[] dialogAccepted = {false};

    final AlertDialog dialog = new AlertDialog.Builder(this)
      .setTitle("Requesting your permission")
      .setView(tv)
      .setPositiveButton("OK", null)
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
          public void onDismiss(DialogInterface d) {
            if (!dialogAccepted[0])
              return;

            final boolean isBluetooth = isBluetoothPermission(permission);
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                  requestPermissionAfterDialog(permission, handler, requestBothBluetooth,
                                               requestBackgroundAfter, isBluetooth);
                }
              });
          }
        })
      .create();

    dialog.setOnShowListener(new DialogInterface.OnShowListener() {
        @Override
        public void onShow(DialogInterface d) {
          dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
              @Override
              public void onClick(View v) {
                dialogAccepted[0] = true;
                dialog.dismiss();
              }
            });
        }
      });

    dialog.show();
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

    /* For ACCESS_FINE_LOCATION, check if we'll also need background location.
       If so, we'll show a separate disclosure for background after foreground
       is granted (as required by Google Play policy). */
    final boolean requestBackgroundAfter =
      isForegroundLocationPermission(permission) &&
      android.os.Build.VERSION.SDK_INT >= 29 &&
      checkSelfPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION) != PackageManager.PERMISSION_GRANTED;

    /* For Bluetooth permissions, check if both SCAN and CONNECT are needed.
       If so, show combined disclosure and request both together. */
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean requestBothBluetooth =
      isBluetoothPerm &&
      android.os.Build.VERSION.SDK_INT >= 31 &&
      ((Manifest.permission.BLUETOOTH_SCAN.equals(permission) &&
        checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) ||
       (Manifest.permission.BLUETOOTH_CONNECT.equals(permission) &&
        checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED));

    mainHandler.post(new Runnable() {
        @Override public void run() {
          showRequestPermissionRationale(permission, rationale, handler, requestBackgroundAfter, requestBothBluetooth);
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
    doRequestPermissions(new String[]{permission}, handler);
  }

  private void doRequestPermissions(String[] permissions,
                                     PermissionHandler handler) {
    if (android.os.Build.VERSION.SDK_INT >= 23) {
      if (isFinishing() || (android.os.Build.VERSION.SDK_INT >= 17 && isDestroyed())) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
        return;
      }

      /* For Bluetooth permissions on Android 12+, verify they're actually needed */
      for (String perm : permissions) {
        if (isBluetoothPermission(perm) &&
            android.os.Build.VERSION.SDK_INT < 31) {
          /* Bluetooth permissions not needed on Android < 12, granted automatically */
          if (handler != null)
            handler.onRequestPermissionsResult(true);
          return;
        }
      }

      int requestCode = addPermissionHandler(handler);

      try {
        requestPermissions(permissions, requestCode);
      } catch (IllegalStateException e) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
      } catch (Exception e) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
      }
    } else if (handler != null) {
      handler.onRequestPermissionsResult(true);
    }
  }

  /* virtual methods from PermissionManager */

  private final Map<Integer, PermissionHandler> permissionHandlers =
    new TreeMap<Integer, PermissionHandler>();
  private int nextPermissionHandlerId = 0;

  /* Track pending Bluetooth permission requests to avoid duplicate disclosures */
  private final java.util.Set<String> pendingBluetoothPermissions =
    new java.util.HashSet<String>();

  /* Permission request queue to prevent overlapping dialogs */
  private static class PendingPermissionRequest {
    final String permission;
    final PermissionHandler handler;

    PendingPermissionRequest(String permission, PermissionHandler handler) {
      this.permission = permission;
      this.handler = handler;
    }
  }

  private final Queue<PendingPermissionRequest> permissionQueue = new LinkedList<>();
  private boolean isProcessingPermission = false;

  @Override
  public boolean requestPermission(String permission, PermissionHandler handler) {
    if (android.os.Build.VERSION.SDK_INT < 23)
      /* we don't need to request permissions on this old Android
         version */
      return true;

    /* Skip permission requests in simulator mode - permissions are not needed */
    if (Loader.loaded && NativeView.isSimulator())
      return true;

    if (checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED)
      return true;

    /* For location, Bluetooth, and notification permissions, always show disclosure dialog before
       requesting permission (required by Google Play policy). For other
       permissions, only show rationale if user previously denied. */
    final boolean isLocationPerm = isLocationPermission(permission);
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean isNotificationPerm = isNotificationPermission(permission);

    /* Queue the permission request to prevent overlapping dialogs */
    permissionQueue.offer(new PendingPermissionRequest(permission, handler));
    processNextPermission();
    return false;
  }

  /**
   * Process the next permission request in the queue.
   * Only one permission request is processed at a time to prevent overlapping dialogs.
   */
  private synchronized void processNextPermission() {
    /* Don't process if already processing or queue is empty */
    if (isProcessingPermission || permissionQueue.isEmpty())
      return;

    final PendingPermissionRequest request = permissionQueue.poll();
    if (request == null)
      return;

    final String permission = request.permission;
    
    /* Skip if permission is already granted */
    if (checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED) {
      if (request.handler != null)
        request.handler.onRequestPermissionsResult(true);
      processNextPermission();
      return;
    }

    isProcessingPermission = true;
    PermissionHandler handler = request.handler;

    /* For location, Bluetooth, and notification permissions, always show disclosure dialog before
       requesting permission (required by Google Play policy). For other
       permissions, only show rationale if user previously denied. */
    final boolean isLocationPerm = isLocationPermission(permission);
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean isNotificationPerm = isNotificationPermission(permission);

    /* For Bluetooth permissions, check if the other one is also in the queue or needed.
       If so, remove it from queue and handle both together. */
    if (isBluetoothPerm && android.os.Build.VERSION.SDK_INT >= 31) {
      final String otherBluetoothPermission =
        Manifest.permission.BLUETOOTH_SCAN.equals(permission) ?
        Manifest.permission.BLUETOOTH_CONNECT :
        Manifest.permission.BLUETOOTH_SCAN;

      boolean otherNeeded = checkSelfPermission(otherBluetoothPermission) != PackageManager.PERMISSION_GRANTED;

      /* Check if the other Bluetooth permission is in the queue */
      PendingPermissionRequest otherRequest = null;
      for (PendingPermissionRequest req : permissionQueue) {
        if (otherBluetoothPermission.equals(req.permission)) {
          otherRequest = req;
          break;
        }
      }

      /* If the other Bluetooth permission is already being processed (in pendingBluetoothPermissions),
         or if it's already granted, skip this one - it will be handled when the other one is processed */
      if (pendingBluetoothPermissions.contains(otherBluetoothPermission) ||
          (!otherNeeded && isBluetoothPermission(permission))) {
        /* Other Bluetooth permission is already being processed or already granted.
           Skip this one and mark as granted since it will be handled together. */
        isProcessingPermission = false;
        if (handler != null)
          handler.onRequestPermissionsResult(true);
        processNextPermission();
        return;
      }

      if (otherRequest != null && otherNeeded) {
        /* Remove the other Bluetooth permission from queue - we'll handle both together */
        permissionQueue.remove(otherRequest);
        pendingBluetoothPermissions.add(permission);
        pendingBluetoothPermissions.add(otherBluetoothPermission);
        /* Update handler to also call the other permission's handler */
        final PermissionHandler originalHandler = handler;
        final PermissionHandler otherHandler = otherRequest.handler;
        handler = new PermissionHandler() {
            @Override
            public void onRequestPermissionsResult(boolean granted) {
              if (originalHandler != null)
                originalHandler.onRequestPermissionsResult(granted);
              if (otherHandler != null)
                otherHandler.onRequestPermissionsResult(granted);
            }
          };
      } else if (otherNeeded) {
        pendingBluetoothPermissions.add(permission);
        pendingBluetoothPermissions.add(otherBluetoothPermission);
      } else if (pendingBluetoothPermissions.contains(otherBluetoothPermission)) {
        pendingBluetoothPermissions.add(permission);
      }
    }

    /* Always show consent dialog for location, Bluetooth, and notification permissions */
    final PermissionHandler finalHandler = handler;
    if ((isLocationPerm || isBluetoothPerm || isNotificationPerm ||
         shouldShowRequestPermissionRationale(permission)) &&
        showRequestPermissionRationaleIndirect(permission, new PermissionHandler() {
            @Override
            public void onRequestPermissionsResult(boolean granted) {
              isProcessingPermission = false;
              if (finalHandler != null)
                finalHandler.onRequestPermissionsResult(granted);
              processNextPermission();
            }
          }))
      return;

    doRequestPermission(permission, new PermissionHandler() {
        @Override
        public void onRequestPermissionsResult(boolean granted) {
          isProcessingPermission = false;
          if (finalHandler != null)
            finalHandler.onRequestPermissionsResult(granted);
          processNextPermission();
        }
      });
  }

  @Override
  public synchronized void cancelRequestPermission(PermissionHandler handler) {
    permissionHandlers.values().remove(handler);
  }
}
