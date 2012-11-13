/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

/**
 * A #View which calls the native part of XCSoar.
 */
class NativeView extends SurfaceView
  implements SurfaceHolder.Callback, Runnable {
  private static final String TAG = "XCSoar";

  Handler quitHandler;

  Resources resources;

  final boolean hasKeyboard;

  EGL10 egl;
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;

  /**
   * Is the extension ARB_texture_non_power_of_two present?  If yes,
   * then textures can have any size, not just power of two.
   */
  static boolean textureNonPowerOfTwo;

  Thread thread;

  public NativeView(Activity context, Handler _quitHandler) {
    super(context);

    quitHandler = _quitHandler;

    resources = context.getResources();

    hasKeyboard = resources.getConfiguration().keyboard !=
      Configuration.KEYBOARD_NOKEYS;

    touchInput = DifferentTouchInput.getInstance();

    SurfaceHolder holder = getHolder();
    holder.addCallback(this);
    holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
  }

  private void start() {
    thread = new Thread(this, "NativeMain");
    thread.start();
  }

  private void initGL(SurfaceHolder holder) {
    /* initialize display */

    egl = (EGL10)EGLContext.getEGL();
    display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

    Log.d(TAG, "EGL vendor: " +
          egl.eglQueryString(display, EGL10.EGL_VENDOR));
    Log.d(TAG, "EGL version: " +
          egl.eglQueryString(display, EGL10.EGL_VERSION));
    Log.d(TAG, "EGL extensions: " +
          egl.eglQueryString(display, EGL10.EGL_EXTENSIONS));

    int[] version = new int[2];
    egl.eglInitialize(display, version);

    /* choose a configuration */

    int[] num_config = new int[1];
    int[] configSpec = new int[]{
      EGL10.EGL_STENCIL_SIZE, 1,  /* Don't change this position in array! */
      EGL10.EGL_RED_SIZE, 4,
      EGL10.EGL_GREEN_SIZE, 4,
      EGL10.EGL_BLUE_SIZE, 4,
      EGL10.EGL_ALPHA_SIZE, 0,
      EGL10.EGL_DEPTH_SIZE, 0,
      EGL10.EGL_NONE
    };

    egl.eglChooseConfig(display, configSpec, null, 0, num_config);
    if (num_config[0] == 0) {
      /* fallback in case stencil buffer is not available */
      configSpec[1] = 0;
      egl.eglChooseConfig(display, configSpec, null, 0, num_config);
    }

    int numConfigs = num_config[0];
    EGLConfig[] configs = new EGLConfig[numConfigs];
    egl.eglChooseConfig(display, configSpec, configs, numConfigs, num_config);

    EGLConfig closestConfig = null;
    int closestDistance = 1000;
    for (EGLConfig config : configs) {
      int r = findConfigAttrib(config, EGL10.EGL_RED_SIZE, 0);
      int g = findConfigAttrib(config, EGL10.EGL_GREEN_SIZE, 0);
      int b = findConfigAttrib(config, EGL10.EGL_BLUE_SIZE, 0);
      int a = findConfigAttrib(config, EGL10.EGL_ALPHA_SIZE, 0);
      int d = findConfigAttrib(config, EGL10.EGL_DEPTH_SIZE, 0);
      int s = findConfigAttrib(config, EGL10.EGL_STENCIL_SIZE, 0);
      int distance = Math.abs(r - 5) + Math.abs(g - 6) + Math.abs(b - 5) +
        Math.abs(a - 0) + Math.abs(d - 0) + Math.abs(s - 8);
      if (distance < closestDistance) {
        closestDistance = distance;
        closestConfig = config;
      }
    }

    Log.d(TAG, "EGLConfig: red="+
          findConfigAttrib(closestConfig, EGL10.EGL_RED_SIZE, 0) +
          " green=" + findConfigAttrib(closestConfig, EGL10.EGL_GREEN_SIZE, 0) +
          " blue=" + findConfigAttrib(closestConfig, EGL10.EGL_BLUE_SIZE, 0) +
          " alpha=" + findConfigAttrib(closestConfig, EGL10.EGL_ALPHA_SIZE, 0) +
          " depth=" + findConfigAttrib(closestConfig, EGL10.EGL_DEPTH_SIZE, 0) +
          " stencil=" + findConfigAttrib(closestConfig, EGL10.EGL_STENCIL_SIZE, 0));

    /* initialize context and surface */

    context = egl.eglCreateContext(display, closestConfig,
                                   EGL10.EGL_NO_CONTEXT, null);

    surface = egl.eglCreateWindowSurface(display, closestConfig,
                                         holder, null);
    egl.eglMakeCurrent(display, surface, surface, context);

    GL10 gl = (GL10)context.getGL();
    Log.d(TAG, "OpenGL vendor: " + gl.glGetString(GL10.GL_VENDOR));
    Log.d(TAG, "OpenGL version: " + gl.glGetString(GL10.GL_VERSION));
    Log.d(TAG, "OpenGL renderer: " + gl.glGetString(GL10.GL_RENDERER));
    Log.d(TAG, "OpenGL extensions: " + gl.glGetString(GL10.GL_EXTENSIONS));
  }

  /**
   * Initializes the OpenGL surface.  Called by the native code.
   */
  private boolean initSurface() {
    try {
      initGL(getHolder());
      return true;
    } catch (Exception e) {
      Log.e(TAG, "initGL error: " + e);
      deinitSurface();
      return false;
    }
  }

  /**
   * Deinitializes the OpenGL surface.
   */
  private void deinitSurface() {
    if (surface != null) {
      egl.eglMakeCurrent(display, EGL10.EGL_NO_SURFACE,
                         EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
      egl.eglDestroySurface(display, surface);
      surface = null;
    }

    if (context != null) {
      egl.eglDestroyContext(display, context);
      context = null;
    }

    if (display != null) {
      egl.eglTerminate(display);
      display = null;
    }
  }

  private boolean setRequestedOrientation(int requestedOrientation) {
    ((Activity)getContext()).setRequestedOrientation(requestedOrientation);
    return true;
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
    initGL(getHolder());

    android.graphics.Rect r = getHolder().getSurfaceFrame();
    DisplayMetrics metrics = new DisplayMetrics();
    ((Activity)getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
    if (initializeNative(getContext(), r.width(), r.height(),
                         (int)metrics.xdpi, (int)metrics.ydpi,
                         Build.VERSION.SDK_INT, Build.PRODUCT))
        runNative();
    Log.d(TAG, "deinitializeNative()");
    deinitializeNative();

    Log.d(TAG, "sending message to quitHandler");
    quitHandler.sendMessage(quitHandler.obtainMessage());
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
    int[] mValue = new int[1];
    return egl.eglGetConfigAttrib(display, config, attribute, mValue)
      ? mValue[0]
      : defaultValue;
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
   * Loads the specified bitmap resource as OpenGL texture.
   *
   * @param result an array of 3 integers: texture id, width, height
   * (all output)
   * @return true on success
   */
  private boolean loadResourceTexture(String name, int[] result) {
    /* find the resource */
    int resourceId = resources.getIdentifier(name, "drawable", "org.xcsoar");
    if (resourceId == 0) {
      resourceId = resources.getIdentifier(name, "drawable",
                                           "org.xcsoar.testing");
      if (resourceId == 0)
        return false;
    }

    /* load the Bitmap from the resource */
    BitmapFactory.Options opts = new BitmapFactory.Options();
    opts.inScaled = false;

    Bitmap bmp = BitmapFactory.decodeResource(resources, resourceId, opts);

    return BitmapUtil.bitmapToOpenGL(bmp, result);
  }

  /**
   * Loads an image from filesystem as OpenGL texture.
   *
   * @param result an array of 3 integers: texture id, width, height
   * (all output)
   * @return true on success
   */
  private boolean loadFileTexture(String pathName, int[] result) {
    /* load the Bitmap from filesystem */
    BitmapFactory.Options opts = new BitmapFactory.Options();
    opts.inScaled = false;

    Bitmap bmp = BitmapFactory.decodeFile(pathName, opts);

    return BitmapUtil.bitmapToOpenGL(bmp, result);
  }

  /**
   * Starts a VIEW intent for a given file
   */
  private void openFile(String pathName) {
    Intent intent = new Intent();
    intent.setAction(Intent.ACTION_VIEW);
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK +
                    Intent.FLAG_RECEIVER_REPLACE_PENDING);
    File file = new File(pathName);

    try {
      String extension = pathName.substring(pathName.lastIndexOf(".") + 1);
      MimeTypeMap mime = MimeTypeMap.getSingleton();
      String mimeType = mime.getMimeTypeFromExtension(extension);

      intent.setDataAndType(Uri.fromFile(file), mimeType);
      getContext().startActivity(intent);
    } catch (Exception e) {
      Log.e(TAG, "NativeView.openFile('" + pathName + "') error: " + e);
    }
  }

  private void swap() {
    egl.eglSwapBuffers(display, surface);
  }

  @Override public boolean onTouchEvent(final MotionEvent event)
  {
    touchInput.process(event);
    return true;
  }

  public void onResume() {
    resumeNative();
  }

  public void onPause() {
    pauseNative();
  }

  public void exitApp() {
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

  DifferentTouchInput touchInput = null;
}
