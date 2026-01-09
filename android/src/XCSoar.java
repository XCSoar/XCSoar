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
      /* some Android don't restore fullscreen settings after returning to
         this app or after orientation changes, so we need to reapply all
         fullscreen settings (immersive mode + display cutout mode) manually */
      WindowUtil.enterFullScreenMode(getWindow());

    super.onWindowFocusChanged(hasFocus);
  }

  @Override
  public void onMultiWindowModeChanged(boolean isInMultiWindowMode) {
    applyFullScreen();
  }

  /**
   * Threshold in pixels from the top edge where swipes should be
   * ignored to allow system status bar gesture.
   */
  private static final int TOP_EDGE_THRESHOLD = 30;

  /**
   * Track the initial Y position of a touch gesture to detect
   * top-edge swipes in fullscreen mode.
   */
  private float initialTouchY = -1;

  @Override public boolean dispatchTouchEvent(final MotionEvent ev) {
    /* In fullscreen mode, detect swipes from the very top edge and
       let the system handle them (to show status bar) instead of
       passing them to the app. */
    if (wantFullScreen() && nativeView != null) {
      final int action = ev.getActionMasked();
      final float y = ev.getY();

      switch (action) {
      case MotionEvent.ACTION_DOWN:
        /* Check if touch starts from the very top edge */
        if (y <= TOP_EDGE_THRESHOLD) {
          initialTouchY = y;
          /* Let the system handle this touch to show status bar */
          return false;
        }
        initialTouchY = -1;
        break;

      case MotionEvent.ACTION_MOVE:
        /* If we started from the top edge and are moving downward,
           continue letting the system handle it */
        if (initialTouchY >= 0 && initialTouchY <= TOP_EDGE_THRESHOLD) {
          final float dy = y - initialTouchY;
          if (dy > 0) {
            /* Moving downward from top edge - let system handle it */
            return false;
          }
          /* Moving upward - cancel the gesture and pass to app */
          initialTouchY = -1;
        }
        break;

      case MotionEvent.ACTION_UP:
      case MotionEvent.ACTION_CANCEL:
        initialTouchY = -1;
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
    
    /* applyFullScreen() will handle updating the native view with the correct size.
       No need to duplicate the logic here - applyFullScreen() already posts to decorView
       to ensure layout is complete before getting the SurfaceView size. */
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
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
              android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(XCSoar.this);
              builder.setTitle("Permission Required");
              builder.setMessage("XCSoar needs " + permissions[0] + " permission, but it was previously denied. " +
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
    if (Manifest.permission.ACCESS_FINE_LOCATION.equals(permission))
      return "XCSoar needs permission to access your GPS location - obviously, because XCSoar's purpose is to help you navigate an aircraft.";
    else if (Manifest.permission.ACCESS_BACKGROUND_LOCATION.equals(permission))
      return getBackgroundLocationRationale();
    else if (Manifest.permission.BLUETOOTH_CONNECT.equals(permission) ||
             Manifest.permission.BLUETOOTH_SCAN.equals(permission))
      return "If you want XCSoar to connect to Bluetooth sensors, it needs your permission.";
    else if (android.os.Build.VERSION.SDK_INT >= 34 &&
             "android.permission.FOREGROUND_SERVICE_LOCATION".equals(permission))
      return "XCSoar needs permission to run as a foreground service with location access to continue tracking your flight when the app is in the background.";
    else
      return null;
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
            /* Request permission after dialog is fully dismissed to prevent overlap */
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                  if (requestBothBluetooth) {
                    /* Request both Bluetooth permissions together */
                    final String[] bluetoothPermissions = {
                      Manifest.permission.BLUETOOTH_SCAN,
                      Manifest.permission.BLUETOOTH_CONNECT
                    };
                    doRequestPermissions(bluetoothPermissions, new PermissionHandler() {
                        @Override
                        public void onRequestPermissionsResult(boolean granted) {
                          pendingBluetoothPermissions.remove(Manifest.permission.BLUETOOTH_SCAN);
                          pendingBluetoothPermissions.remove(Manifest.permission.BLUETOOTH_CONNECT);
                          if (handler != null)
                            handler.onRequestPermissionsResult(granted);
                        }
                      });
                  } else {
                    final boolean isBluetooth = Manifest.permission.BLUETOOTH_SCAN.equals(permission) ||
                                                Manifest.permission.BLUETOOTH_CONNECT.equals(permission);
                    if (isBluetooth)
                      pendingBluetoothPermissions.add(permission);
                    doRequestPermission(permission, new PermissionHandler() {
                        @Override
                        public void onRequestPermissionsResult(boolean granted) {
                          if (isBluetooth)
                            pendingBluetoothPermissions.remove(permission);
                          if (handler != null)
                            handler.onRequestPermissionsResult(granted);

                          /* Request background location with disclosure if needed */
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
                        }
                      });
                  }
                }
              });
          }
        })
      .create();

    /* Set OK button to dismiss dialog - onDismiss will handle permission request */
    dialog.setOnShowListener(new DialogInterface.OnShowListener() {
        @Override
        public void onShow(DialogInterface d) {
          dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
              @Override
              public void onClick(View v) {
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
      Manifest.permission.ACCESS_FINE_LOCATION.equals(permission) &&
      android.os.Build.VERSION.SDK_INT >= 29 &&
      checkSelfPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION) != PackageManager.PERMISSION_GRANTED;

    /* For Bluetooth permissions, check if both SCAN and CONNECT are needed.
       If so, show combined disclosure and request both together. */
    final boolean isBluetoothPermission =
      Manifest.permission.BLUETOOTH_SCAN.equals(permission) ||
      Manifest.permission.BLUETOOTH_CONNECT.equals(permission);
    final boolean requestBothBluetooth =
      isBluetoothPermission &&
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
      /* Check if activity is in valid state */
      if (isFinishing() || (android.os.Build.VERSION.SDK_INT >= 17 && isDestroyed())) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
        return;
      }

      /* For Bluetooth permissions on Android 12+, verify they're actually needed */
      for (String perm : permissions) {
        if ((perm.equals(Manifest.permission.BLUETOOTH_SCAN) || 
             perm.equals(Manifest.permission.BLUETOOTH_CONNECT)) &&
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
      /* On older Android versions, permissions are granted at install time */
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

  @Override
  public boolean requestPermission(String permission, PermissionHandler handler) {
    if (android.os.Build.VERSION.SDK_INT < 23)
      /* we don't need to request permissions on this old Android
         version */
      return true;

    if (checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED)
      /* we already have the permission */
      return true;

    /* For location and Bluetooth permissions, always show disclosure dialog before
       requesting permission (required by Google Play policy). For other
       permissions, only show rationale if user previously denied. */
    final boolean isForegroundLocation =
      Manifest.permission.ACCESS_FINE_LOCATION.equals(permission);
    final boolean isBackgroundLocation =
      Manifest.permission.ACCESS_BACKGROUND_LOCATION.equals(permission);
    final boolean isLocationPermission = isForegroundLocation || isBackgroundLocation;
    final boolean isBluetoothPermission =
      Manifest.permission.BLUETOOTH_CONNECT.equals(permission) ||
      Manifest.permission.BLUETOOTH_SCAN.equals(permission);
    final boolean isForegroundServiceLocation =
      android.os.Build.VERSION.SDK_INT >= 34 &&
      "android.permission.FOREGROUND_SERVICE_LOCATION".equals(permission);

    /* For Bluetooth permissions, check if the other one is also pending or needed.
       If so, we'll show a combined disclosure for both. */
    if (isBluetoothPermission && android.os.Build.VERSION.SDK_INT >= 31) {
      final String otherBluetoothPermission =
        Manifest.permission.BLUETOOTH_SCAN.equals(permission) ?
        Manifest.permission.BLUETOOTH_CONNECT :
        Manifest.permission.BLUETOOTH_SCAN;

      /* Check if other permission is pending (being requested) or also needed */
      boolean otherPending = pendingBluetoothPermissions.contains(otherBluetoothPermission);
      boolean otherNeeded = checkSelfPermission(otherBluetoothPermission) != PackageManager.PERMISSION_GRANTED;

      if (otherPending) {
        /* Other permission is already being requested, skip disclosure for this one */
        pendingBluetoothPermissions.add(permission);
        doRequestPermission(permission, new PermissionHandler() {
            @Override
            public void onRequestPermissionsResult(boolean granted) {
              pendingBluetoothPermissions.remove(permission);
              if (handler != null)
                handler.onRequestPermissionsResult(granted);
            }
          });
        return false;
      } else if (otherNeeded) {
        /* Both are needed, will show combined disclosure */
        pendingBluetoothPermissions.add(permission);
        pendingBluetoothPermissions.add(otherBluetoothPermission);
      }
    }

    if ((isLocationPermission || isBluetoothPermission || isForegroundServiceLocation ||
         shouldShowRequestPermissionRationale(permission)) &&
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
