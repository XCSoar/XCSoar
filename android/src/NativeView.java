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
   * A native pointer to a C++ #TopWindow instance.
   */
  private long ptr;

  final Handler quitHandler, wakelockhandler, fullScreenHandler, errorHandler;

  Resources resources;

  final boolean hasKeyboard;

  /**
   * Is the extension ARB_texture_non_power_of_two present?  If yes,
   * then textures can have any size, not just power of two.
   */
  static boolean textureNonPowerOfTwo;

  Thread thread;

  public NativeView(Activity context, Handler _quitHandler,
                    Handler _wakeLockHandler,
                    Handler _fullScreenHandler,
                    Handler _errorHandler) {
    super(context);

    quitHandler = _quitHandler;
    wakelockhandler = _wakeLockHandler;
    fullScreenHandler = _fullScreenHandler;
    errorHandler = _errorHandler;

    resources = context.getResources();

    hasKeyboard = resources.getConfiguration().keyboard !=
      Configuration.KEYBOARD_NOKEYS;

    SurfaceHolder holder = getHolder();
    holder.addCallback(this);
    holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
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
  }

  @Override public void surfaceChanged(SurfaceHolder holder, int format,
                                       int width, int height) {
    if (thread == null || !thread.isAlive())
      start();
    else
      resizedNative(width, height);
  }

  @Override public void surfaceDestroyed(SurfaceHolder holder) {
  }

  @Override public void run() {
    final Context context = getContext();

    android.graphics.Rect r = getHolder().getSurfaceFrame();
    DisplayMetrics metrics = new DisplayMetrics();
    ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(metrics);

    try {
      try {
        context.startService(new Intent(context, XCSoar.serviceClass));
      } catch (IllegalStateException e) {
        /* we get crash reports on this all the time, but I don't
           know why - Android docs say "the application is in a
           state where the service can not be started (such as not
           in the foreground in a state when services are allowed)",
           but we're about to be resumed, which means we're in
           foreground... */
      }

      try {
        runNative(context, r.width(), r.height(),
                  (int)metrics.xdpi, (int)metrics.ydpi,
                  Build.VERSION.SDK_INT, Build.PRODUCT);
      } finally {
        context.stopService(new Intent(context, XCSoar.serviceClass));
      }
    } catch (Exception e) {
      Log.e(TAG, "Initialisation error", e);
      errorHandler.sendMessage(errorHandler.obtainMessage(0, e));
      return;
    }

    quitHandler.sendEmptyMessage(0);
  }

  protected native void runNative(Context context,
                                  int width, int height,
                                  int xdpi, int ydpi,
                                  int sdk_version, String product);

  protected native void resizedNative(int width, int height);

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
    /* find the resource */
    int resourceId = resources.getIdentifier(name, "drawable", "org.xcsoar");
    if (resourceId == 0) {
      resourceId = resources.getIdentifier(name, "drawable",
                                           "org.xcsoar.testing");
      if (resourceId == 0)
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
        .authority("org.xcsoar")
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

    final int x = (int)(event.getX() - offsetX);
    final int y = (int)(event.getY() - offsetY);

    switch (event.getActionMasked()) {
    case MotionEvent.ACTION_DOWN:
      EventBridge.onMouseDown(x, y);
      break;

    case MotionEvent.ACTION_UP:
      EventBridge.onMouseUp(x, y);
      break;

    case MotionEvent.ACTION_MOVE:
      EventBridge.onMouseMove(x, y);
      break;

    case MotionEvent.ACTION_POINTER_DOWN:
      EventBridge.onPointerDown();
      break;

    case MotionEvent.ACTION_POINTER_UP:
      EventBridge.onPointerUp();
      break;
    }

    return true;
  }

  public void onResume() {
    resumeNative();
  }

  public void onPause() {
    pauseNative();
  }

  private final int translateKeyCode(int keyCode) {
    if (!hasKeyboard) {
      /* map the volume keys to cursor up/down if the device has no
         hardware keys */

      switch (keyCode) {
      case KeyEvent.KEYCODE_VOLUME_UP:
        return KeyEvent.KEYCODE_DPAD_UP;

      case KeyEvent.KEYCODE_VOLUME_DOWN:
        return KeyEvent.KEYCODE_DPAD_DOWN;
      }
    }

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
