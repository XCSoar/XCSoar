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

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import java.io.File;
import android.util.Log;
import android.util.DisplayMetrics;
import android.app.Activity;
import android.view.MotionEvent;
import android.view.KeyEvent;
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
import android.opengl.EGL14;
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

  final Handler quitHandler, wakelockhandler, fullScreenHandler, errorHandler;

  Resources resources;

  final boolean hasKeyboard;

  EGL10 egl;
  EGLDisplay display = EGL10.EGL_NO_DISPLAY;
  EGLConfig config;
  EGLContext context = EGL10.EGL_NO_CONTEXT;
  EGLSurface surface = EGL10.EGL_NO_SURFACE;

  /**
   * A 1x1 pbuffer surface that is used to activate the EGLContext
   * while we have no real surface.
   */
  EGLSurface dummySurface = EGL10.EGL_NO_SURFACE;

  /**
   * Is the EGLSurface currently valid?  This is modified by
   * SurfaceHolder.Callback methods.
   */
  boolean haveSurface = false;

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

  private static EGLConfig chooseEglConfig(EGL10 egl, EGLDisplay display)
    throws EGLException {
    int[] num_config = new int[1];
    int[] configSpec = new int[]{
      /* EGL_STENCIL_SIZE not listed here because we have a fallback
         for configurations without stencil (but we prefer native
         stencil) (maybe we can just require a stencil and get rid of
         the complicated and slow fallback code eventually?) */

      EGL10.EGL_RED_SIZE, 4,
      EGL10.EGL_GREEN_SIZE, 4,
      EGL10.EGL_BLUE_SIZE, 4,

      EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT,
      EGL10.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,

      EGL10.EGL_NONE
    };

    egl.eglChooseConfig(display, configSpec, null, 0, num_config);

    int numConfigs = num_config[0];
    EGLConfig[] configs = new EGLConfig[numConfigs];
    if (!egl.eglChooseConfig(display, configSpec,
                             configs, numConfigs, num_config))
      throw new EGLException("eglChooseConfig() failed: " + egl.eglGetError());

    EGLConfig closestConfig = EGLUtil.findClosestConfig(egl, display, configs,
                                                        5, 6, 5, 0, 0, 1);
    if (closestConfig == null)
      throw new EGLException("eglChooseConfig() failed");

    return closestConfig;
  }

  private void initGL(SurfaceHolder holder) throws EGLException {
    /* initialize display */

    if (display == EGL10.EGL_NO_DISPLAY) {
      egl = (EGL10)EGLContext.getEGL();
      display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
      if (display == EGL10.EGL_NO_DISPLAY)
        throw new EGLException("eglGetDisplay() failed");

      int[] version = new int[2];
      if (!egl.eglInitialize(display, version))
        throw new EGLException("eglInitialize() failed: " + egl.eglGetError());

      Log.d(TAG, "EGL vendor: " +
            egl.eglQueryString(display, EGL10.EGL_VENDOR));
      Log.d(TAG, "EGL version: " +
            egl.eglQueryString(display, EGL10.EGL_VERSION));
      Log.d(TAG, "EGL extensions: " +
            egl.eglQueryString(display, EGL10.EGL_EXTENSIONS));
    }

    /* choose a configuration */

    if (config == null) {
      config = chooseEglConfig(egl, display);
      Log.d(TAG, "EGLConfig = " + EGLUtil.toString(egl, display, config));
    }

    /* initialize context and surface */

    boolean hadContext = context != EGL10.EGL_NO_CONTEXT;
    if (!hadContext) {
      final int contextClientVersion = 2;
      final int[] contextAttribList = new int[]{
        EGL14.EGL_CONTEXT_CLIENT_VERSION, contextClientVersion,
        EGL10.EGL_NONE
      };

      context = egl.eglCreateContext(display, config,
                                     EGL10.EGL_NO_CONTEXT, contextAttribList);
      if (context == EGL10.EGL_NO_CONTEXT)
        throw new EGLException("eglCreateContext() failed: " +
                               egl.eglGetError());
    }

    surface = egl.eglCreateWindowSurface(display, config,
                                         holder, null);
    if (surface == EGL10.EGL_NO_SURFACE)
      throw new EGLException("eglCreateWindowSurface() failed: " +
                             egl.eglGetError());

    if (!egl.eglMakeCurrent(display, surface, surface, context))
      throw new EGLException("eglMakeCurrent() failed: " + egl.eglGetError());

    if (!hadContext) {
      GL10 gl = (GL10)context.getGL();
      Log.d(TAG, "OpenGL vendor: " + gl.glGetString(GL10.GL_VENDOR));
      Log.d(TAG, "OpenGL version: " + gl.glGetString(GL10.GL_VERSION));
      Log.d(TAG, "OpenGL renderer: " + gl.glGetString(GL10.GL_RENDERER));
      Log.d(TAG, "OpenGL extensions: " + gl.glGetString(GL10.GL_EXTENSIONS));
    }
  }

  /**
   * Initializes the OpenGL surface.  Called by the native code.
   */
  private boolean initSurface() {
    if (!haveSurface)
      /* this is futile, and will only result in
         "java.lang.IllegalArgumentException: Make sure the
         SurfaceView or associated SurfaceHolder has a valid
         Surface" */
      return false;

    try {
      initGL(getHolder());
      return true;
    } catch (Exception e) {
      Log.e(TAG, "initGL error", e);
      deinitSurface();
      return false;
    }
  }

  /**
   * Deinitializes the OpenGL surface.
   */
  private void deinitSurface() {
    if (surface != EGL10.EGL_NO_SURFACE) {
      if (dummySurface == EGL10.EGL_NO_SURFACE) {
        int pbufferAttribs[] = {
          EGL10.EGL_WIDTH, 1,
          EGL10.EGL_HEIGHT, 1,
          EGL10.EGL_NONE
        };

        dummySurface = egl.eglCreatePbufferSurface(display, config,
                                                   pbufferAttribs);
      }

      egl.eglMakeCurrent(display, dummySurface, dummySurface, context);
      egl.eglDestroySurface(display, surface);
      surface = EGL10.EGL_NO_SURFACE;
    }
  }

  private void deinitEGL() {
    deinitSurface();

    if (context != EGL10.EGL_NO_CONTEXT) {
      egl.eglDestroyContext(display, context);
      context = EGL10.EGL_NO_CONTEXT;
    }

    if (display != EGL10.EGL_NO_DISPLAY) {
      egl.eglTerminate(display);
      display = EGL10.EGL_NO_DISPLAY;
    }

    config = null;
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
    haveSurface = true;
  }

  @Override public void surfaceChanged(SurfaceHolder holder, int format,
                                       int width, int height) {
    haveSurface = true;

    if (thread == null || !thread.isAlive())
      start();
    else
      resizedNative(width, height);
  }

  @Override public void surfaceDestroyed(SurfaceHolder holder) {
    haveSurface = false;
  }

  @Override public void run() {
    final Context context = getContext();

    try {
      initGL(getHolder());
    } catch (Exception e) {
      Log.e(TAG, "initGL error", e);
      errorHandler.sendMessage(errorHandler.obtainMessage(0, e));
      deinitEGL();
      return;
    }

    android.graphics.Rect r = getHolder().getSurfaceFrame();
    DisplayMetrics metrics = new DisplayMetrics();
    ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(metrics);

    try {
      if (initializeNative(context, r.width(), r.height(),
                           (int)metrics.xdpi, (int)metrics.ydpi,
                           Build.VERSION.SDK_INT, Build.PRODUCT)) {

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
          runNative();
        } finally {
          context.stopService(new Intent(context, XCSoar.serviceClass));
        }
      }
    } catch (Exception e) {
      Log.e(TAG, "Initialisation error", e);
      errorHandler.sendMessage(errorHandler.obtainMessage(0, e));
      deinitEGL();
      return;
    }

    deinitializeNative();
    quitHandler.sendEmptyMessage(0);
  }

  protected native boolean initializeNative(Context context,
                                            int width, int height,
                                            int xdpi, int ydpi,
                                            int sdk_version, String product);
  protected native void runNative();
  protected native void deinitializeNative();
  protected native void resizedNative(int width, int height);

  protected native void pauseNative();
  protected native void resumeNative();

  protected native void setBatteryPercent(int level, int plugged);

  protected native void setHapticFeedback(boolean on);

  private int findConfigAttrib(EGLConfig config, int attribute,
                               int defaultValue) {
    return EGLUtil.getConfigAttrib(egl, display, config,
                                   attribute, defaultValue);
  }

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
