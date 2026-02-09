// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.File;
import android.util.Log;
import android.util.DisplayMetrics;
import android.app.Activity;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewParent;
import android.view.Window;
import android.os.Build;
import android.os.Handler;
import android.net.Uri;
import android.content.Intent;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.webkit.MimeTypeMap;

class EGLException extends Exception {
  private static final long serialVersionUID = 5928634879321047581L;

  public EGLException(String _msg) {
    super(_msg);
  }
};

/**
 * A #View which calls the native part of XCSoar.
 */
class NativeView extends SurfaceView
  implements SurfaceHolder.Callback, Runnable {
  private static final String TAG = "XCSoar";

  /**
   * Filters touch events to reject system edge gestures.
   */
  private final EdgeTouchFilter edgeTouchFilter = new EdgeTouchFilter();

  /**
   * A native pointer to a C++ #TopWindow instance.
   */
  private long ptr;

  final PermissionManager permissionManager;

  final Handler quitHandler, wakelockhandler, fullScreenHandler, errorHandler;

  Resources resources;

  final boolean hasKeyboard;

  /**
   * Is the extension ARB_texture_non_power_of_two present?  If yes,
   * then textures can have any size, not just power of two.
   */
  static boolean textureNonPowerOfTwo;

  Thread thread;

  /**
   * Listens for physical device orientation changes to offer a
   * rotation suggestion button (like Android's Rotate Suggestions).
   */
  private RotationListener rotationListener;

  /*
   * Check if running in simulator mode (user chose "simulator" on startup)
   */
  private static native boolean isSimulatorNative();

  public static boolean isSimulator() {
    try {
      return isSimulatorNative();
    } catch (UnsatisfiedLinkError e) {
      return false;
    }
  }

  public NativeView(Activity context, Handler _quitHandler,
                    Handler _wakeLockHandler,
                    Handler _fullScreenHandler,
                    Handler _errorHandler,
                    PermissionManager permissionManager) {
    super(context);

    quitHandler = _quitHandler;
    wakelockhandler = _wakeLockHandler;
    fullScreenHandler = _fullScreenHandler;
    errorHandler = _errorHandler;
    this.permissionManager = permissionManager;

    resources = context.getResources();

    hasKeyboard = resources.getConfiguration().keyboard !=
      Configuration.KEYBOARD_NOKEYS;

    SurfaceHolder holder = getHolder();
    holder.addCallback(this);
    holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);

    setOnApplyWindowInsetsListener(edgeTouchFilter);
    requestApplyInsets(); // trigger initial inset calculation

    rotationListener = new RotationListener(context);
  }

  private void start() {
    thread = new Thread(this, "NativeMain");
    thread.start();
  }

  /**
   * Called from TopCanvas::AcquireSurface() (native code).
   */
  private Surface getSurface() {
    return getHolder().getSurface();
  }

  /**
   * Called from native code.
   */
  void acquireWakeLock() {
    wakelockhandler.sendEmptyMessage(0);
  }

  /**
   * Called from native code.
   */
  void setFullScreen(boolean fullScreen) {
    fullScreenHandler.sendEmptyMessage(fullScreen ? 1 : 0);
  }

  /**
   * Check if the system auto-rotate setting is enabled.
   * Called from native code.
   */
  private boolean isAutoRotateEnabled() {
    try {
      return android.provider.Settings.System.getInt(
        getContext().getContentResolver(),
        android.provider.Settings.System.ACCELEROMETER_ROTATION) == 1;
    } catch (Exception e) {
      return false;
    }
  }

  private boolean setRequestedOrientation(int requestedOrientation) {
    try {
      ((Activity)getContext()).setRequestedOrientation(requestedOrientation);
      return true;
    } catch (Exception e) {
      /* even though undocumented, there are reports of
         setRequestedOrientation() throwing IllegalStateException */
      Log.e(TAG, "setRequestedOrientation error", e);
      return false;
    }
  }

  @Override public void surfaceCreated(SurfaceHolder holder) {
    if (rotationListener != null && rotationListener.canDetectOrientation())
      rotationListener.enable();
  }

  @Override public void surfaceChanged(SurfaceHolder holder, int format,
                                       int width, int height) {
    if (thread == null || !thread.isAlive())
      start();
    else {
      Context context = getContext();
      if (!(context instanceof Activity))
        return;
      
      Activity activity = (Activity)context;
      Window window = activity.getWindow();
      View decorView = window.getDecorView();
      
      boolean is_fullscreen = false;
      if (activity instanceof org.xcsoar.XCSoar) {
        is_fullscreen = ((org.xcsoar.XCSoar)activity).wantFullScreen();
      } else {
        int systemUiVisibility = decorView.getSystemUiVisibility();
        is_fullscreen = (systemUiVisibility & View.SYSTEM_UI_FLAG_FULLSCREEN) != 0 ||
                       (systemUiVisibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) != 0;
      }
      
      if (is_fullscreen) {
        WindowUtil.enterFullScreenMode(window);
      } else {
        WindowUtil.leaveFullScreenMode(window, 0);
      }
      
      int display_width = width;
      int display_height = height;
      int inset_left = 0, inset_top = 0, inset_right = 0, inset_bottom = 0;
      
      resizedNative(display_width, display_height, inset_left, inset_top, inset_right, inset_bottom);
    }
  }

  @Override public void surfaceDestroyed(SurfaceHolder holder) {
    if (rotationListener != null)
      rotationListener.disable();

    surfaceDestroyedNative();
  }

  @Override public void run() {
    final Context context = getContext();

    android.graphics.Rect r = getHolder().getSurfaceFrame();
    android.util.Log.d(TAG, "runNative: getSurfaceFrame() size=" + r.width() + "x" + r.height());
    DisplayMetrics metrics = new DisplayMetrics();
    ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(metrics);

    try {
      try {
        /* On Android 14+ (API 34+), FOREGROUND_SERVICE_LOCATION is a runtime permission
           that must be granted before starting a foreground service with type="location" */
        boolean serviceStarted = false;
        if (Build.VERSION.SDK_INT >= 34) {
          final String fgsPermission = "android.permission.FOREGROUND_SERVICE_LOCATION";
          if (context.checkSelfPermission(fgsPermission) ==
              android.content.pm.PackageManager.PERMISSION_GRANTED) {
            context.startService(new Intent(context, MyService.class));
            serviceStarted = true;
          } else {
            /* Permission not granted - request it. Service will be started after permission is granted.
               For now, try to start it anyway - it will fail gracefully with SecurityException if needed. */
            permissionManager.requestPermission(fgsPermission, null);
            /* Try to start service anyway - on some devices it might work, or will fail with SecurityException */
            try {
              context.startService(new Intent(context, MyService.class));
              serviceStarted = true;
            } catch (SecurityException e) {
              /* Expected on Android 14+ without permission - service will be started after permission is granted */
            }
          }
        } else {
          context.startService(new Intent(context, MyService.class));
          serviceStarted = true;
        }
      } catch (IllegalStateException e) {
        /* we get crash reports on this all the time, but I don't
           know why - Android docs say "the application is in a
           state where the service can not be started (such as not
           in the foreground in a state when services are allowed)",
           but we're about to be resumed, which means we're in
           foreground... */
      } catch (SecurityException e) {
        /* On Android 14+ without FOREGROUND_SERVICE_LOCATION permission, starting the service throws SecurityException.
           This is expected - the service will be started after permission is granted via the permission request above. */
      }

      try {
        runNative(context, permissionManager,
                  r.width(), r.height(),
                  (int)metrics.xdpi, (int)metrics.ydpi,
                  Build.PRODUCT);
      } finally {
        context.stopService(new Intent(context, MyService.class));
      }
    } catch (Exception e) {
      Log.e(TAG, "Initialisation error", e);
      errorHandler.sendMessage(errorHandler.obtainMessage(0, e));
      return;
    }

    quitHandler.sendEmptyMessage(0);
  }

  static native void initNative();
  static native void deinitNative();

  /**
   * Show a native permission disclosure dialog on the XCSoar UI
   * thread.  Called from PermissionHelper instead of showing a Java
   * AlertDialog.  When the user responds, calls back to
   * PermissionManager.onDisclosureResult().
   */
  static native void showPermissionDisclosure(String permission);

  /**
   * Notify native code that a permission request completed.
   * Called from PermissionHelper when a permission chain finishes.
   *
   * @param granted true if the permission was granted
   */
  static native void onPermissionResult(boolean granted);

  static native void onConfigurationChangedNative(boolean nightMode);

  /**
   * Called when the physical device orientation changes, to show
   * or refresh the rotate suggestion button.
   */
  static native void onRotationSuggestion();

  /**
   * Delegate to {@link RotationListener#getPhysicalOrientation()}.
   * Called from native code when the rotate button is pressed.
   */
  int getPhysicalOrientation() {
    return rotationListener != null
      ? rotationListener.getPhysicalOrientation() : 0;
  }

  static native String onReceiveXCTrackTask(String data);

  protected native void runNative(Context context,
                                  PermissionManager permissionManager,
                                  int width, int height,
                                  int xdpi, int ydpi,
                                  String product);

  protected native void resizedNative(int width, int height, int inset_left, int inset_top, int inset_right, int inset_bottom);

  protected native void surfaceDestroyedNative();

  protected native void pauseNative();
  protected native void resumeNative();

  protected native void setBatteryPercent(int level, int plugged);

  protected native void setHapticFeedback(boolean on);

  /**
   * Finds the next power of two.  Used to calculate texture sizes.
   */
  public static int nextPowerOfTwo(int i) {
    int p = 1;
    while (p < i)
      p <<= 1;
    return p;
  }

  public static int validateTextureSize(int i) {
    return textureNonPowerOfTwo
      ? i
      : nextPowerOfTwo(i);
  }

  /**
   * Loads the specified bitmap resource.
   */
  private Bitmap loadResourceBitmap(String name) {
    /* find the resource using the actual package name */
    String packageName = getContext().getPackageName();
    int resourceId = resources.getIdentifier(name, "drawable", packageName);
    if (resourceId == 0) {
      Log.e(TAG, "Resource not found: drawable/" + name + " in package " + packageName);
      return null;
    }

    /* load the Bitmap from the resource */
    BitmapFactory.Options opts = new BitmapFactory.Options();
    opts.inScaled = false;

    try {
      return BitmapFactory.decodeResource(resources, resourceId, opts);
    } catch (IllegalArgumentException e) {
      Log.e(TAG, "Failed to load bitmap", e);
      return null;
    }
  }

  /**
   * Loads an image from filesystem.
   */
  private Bitmap loadFileBitmap(String pathName) {
    /* load the Bitmap from filesystem */
    BitmapFactory.Options opts = new BitmapFactory.Options();
    opts.inScaled = false;

    return BitmapFactory.decodeFile(pathName, opts);
  }

  /**
   * Loads the specified #Bitmap as OpenGL texture.
   *
   * @param alpha expect a GL_ALPHA texture?
   * @param result an array of 5 integers: texture id, width, height,
   * allocated width, allocated height (all output)
   * @return true on success
   */
  private boolean bitmapToTexture(Bitmap bmp, boolean alpha, int[] result) {
    /* pass a copy because bitmapToOpenGL() recycles the given
       Bitmap */
    return BitmapUtil.bitmapToOpenGL(bmp, false, alpha, result);
  }

  private void shareText(String text) {
    Intent send = new Intent();
    send.setAction(Intent.ACTION_SEND);
    send.putExtra(Intent.EXTRA_TEXT, text);
    send.setType("text/plain");

    Intent share = Intent.createChooser(send, null);
    getContext().startActivity(share);
  }

  /**
   * Opens a URL in the default browser.
   */
  private boolean openURL(String url) {
    try {
      Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      getContext().startActivity(intent);
      return true;
    } catch (Exception e) {
      Log.e(TAG, "openURL('" + url + "') error", e);
      return false;
    }
  }

  /**
   * Starts a VIEW intent for a given file
   */
  private void openWaypointFile(int id, String filename) {
    Intent intent = new Intent();
    intent.setAction(Intent.ACTION_VIEW);
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK +
                    Intent.FLAG_RECEIVER_REPLACE_PENDING);

    try {
      String extension = filename.substring(filename.lastIndexOf(".") + 1);
      MimeTypeMap mime = MimeTypeMap.getSingleton();
      String mimeType = mime.getMimeTypeFromExtension(extension);

      /* this URI is going to be handled by FileProvider */
      Uri uri = new Uri.Builder().scheme("content")
        .authority(getContext().getPackageName())
        .encodedPath("/waypoints/" + id + "/" + Uri.encode(filename))
        .build();

      intent.setDataAndType(uri, mimeType);
      intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

      getContext().startActivity(intent);
    } catch (Exception e) {
      Log.e(TAG, "NativeView.openFile('" + filename + "') error", e);
    }
  }

  private int getNetState() {
    return NetUtil.getNetState();
  }

  @Override public boolean onTouchEvent(final MotionEvent event)
  {
    /* the MotionEvent coordinates are supposed to be relative to this
       View, but in fact they are not: they seem to be relative to
       this app's Window; to work around this, we apply an offset;
       this.getXY() (which is usually 0) plus getParent().getXY()
       (which is a FrameLayout with non-zero coordinates unless we're
       in full-screen mode) */
    float offsetX = getX(), offsetY = getY();
    ViewParent _p = getParent();
    if (_p instanceof View) {
      View p = (View)_p;
      offsetX += p.getX();
      offsetY += p.getY();
    }

    float x = event.getX() - offsetX;
    float y = event.getY() - offsetY;

    edgeTouchFilter.onTouchEvent(event, x, y);
    return true;
  }

  public void onResume() {
    resumeNative();
  }

  public void onPause() {
    pauseNative();
  }

  private static int translateKeyCode(int keyCode) {
    return keyCode;
  }

  @Override public boolean onKeyDown(int keyCode, final KeyEvent event) {
    EventBridge.onKeyDown(translateKeyCode(keyCode));
    return true;
  }

  @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
    EventBridge.onKeyUp(translateKeyCode(keyCode));
    return true;
  }
}
