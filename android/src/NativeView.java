/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

import android.util.Log;
import android.app.Activity;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.os.Build;
import android.os.Handler;
import android.content.res.Resources;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLUtils;

class EventBridge {
  public static native void onKeyDown(int keyCode);
  public static native void onKeyUp(int keyCode);
  public static native void onMouseDown(int x, int y);
  public static native void onMouseUp(int x, int y);
  public static native void onMouseMove(int x, int y);
}

abstract class DifferentTouchInput
{
  public static DifferentTouchInput getInstance()
  {
    if (Integer.parseInt(Build.VERSION.SDK) <= 4)
      return SingleTouchInput.Holder.sInstance;
    else
      return MultiTouchInput.Holder.sInstance;
  }
  public abstract void process(final MotionEvent event);
  private static class SingleTouchInput extends DifferentTouchInput
  {
    private static class Holder
    {
      private static final SingleTouchInput sInstance = new SingleTouchInput();
    }
    public void process(final MotionEvent event)
    {
      if( event.getAction() == MotionEvent.ACTION_DOWN )
        EventBridge.onMouseDown((int)event.getX(), (int)event.getY());
      if( event.getAction() == MotionEvent.ACTION_UP )
        EventBridge.onMouseUp((int)event.getX(), (int)event.getY());
      if( event.getAction() == MotionEvent.ACTION_MOVE )
        EventBridge.onMouseMove((int)event.getX(), (int)event.getY());
    }
  }
  private static class MultiTouchInput extends DifferentTouchInput
  {
    private static class Holder
    {
      private static final MultiTouchInput sInstance = new MultiTouchInput();
    }
    public void process(final MotionEvent event)
    {
        int action = -1;
        if( event.getAction() == MotionEvent.ACTION_DOWN )
          EventBridge.onMouseDown((int)event.getX(), (int)event.getY());
        if( event.getAction() == MotionEvent.ACTION_UP )
          EventBridge.onMouseUp((int)event.getX(), (int)event.getY());
        if( event.getAction() == MotionEvent.ACTION_MOVE )
          EventBridge.onMouseMove((int)event.getX(), (int)event.getY());
    }
  }
}

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
  GL10 gl;

  Thread thread;

  public NativeView(Activity context, Handler _quitHandler) {
    super(context);

    quitHandler = _quitHandler;

    resources = context.getResources();

    hasKeyboard = resources.getConfiguration().keyboard !=
      Configuration.KEYBOARD_NOKEYS;
    Log.d(TAG, "KEYBOARD="+resources.getConfiguration().keyboard);

    touchInput = DifferentTouchInput.getInstance();

    SurfaceHolder holder = getHolder();
    holder.addCallback(this);
    holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
  }

  private void start() {
    thread = new Thread(this);
    thread.start();
  }

  private void initGL(SurfaceHolder holder) {
    /* initialize display */

    egl = (EGL10)EGLContext.getEGL();
    display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

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

    gl = (GL10)context.getGL();
    Log.d(TAG, "OpenGL renderer: " + gl.glGetString(GL10.GL_RENDERER));
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
    if (initializeNative(r.width(), r.height()))
        runNative();
    deinitializeNative();

    quitHandler.sendMessage(quitHandler.obtainMessage());
  }

  protected native boolean initializeNative(int width, int height);
  protected native void runNative();
  protected native void deinitializeNative();
  protected native void resizedNative(int width, int height);

  protected native void pauseNative();
  protected native void resumeNative();

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
  private static int nextPowerOfTwo(int i) {
    int p = 1;
    while (p < i)
      p <<= 1;
    return p;
  }

  /**
   * Initialize the current texture and load the specified Bitmap into
   * it.
   */
  private void loadTexture(Bitmap bmp) {
    /* try the easy way - however this fails on Android 2.2; Android
       1.6 and 2.3 are doing fine */

    GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bmp, 0);
    if (gl.glGetError() != gl.GL_INVALID_VALUE)
      /* success */
      return;

    /* size must be a power of two; create an empty texture, and load
       the Bitmap into it */

    gl.glTexImage2D(GL10.GL_TEXTURE_2D, 0, GL10.GL_RGBA,
                    nextPowerOfTwo(bmp.getWidth()),
                    nextPowerOfTwo(bmp.getHeight()),
                    0, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, null);
    GLUtils.texSubImage2D(GL10.GL_TEXTURE_2D, 0, 0, 0, bmp);
    if (gl.glGetError() != gl.GL_INVALID_OPERATION)
      /* success */
      return;

    /* the two attempts above fail on the Samsung Galaxy Tab; the
       following has been verified to work */

    Bitmap tmp = bmp.copy(Bitmap.Config.RGB_565, false);
    if (tmp == null)
      return;

    gl.glTexImage2D(GL10.GL_TEXTURE_2D, 0, GL10.GL_RGB,
                    nextPowerOfTwo(bmp.getWidth()),
                    nextPowerOfTwo(bmp.getHeight()),
                    0, GL10.GL_RGB, GL10.GL_UNSIGNED_BYTE, null);
    GLUtils.texSubImage2D(GL10.GL_TEXTURE_2D, 0, 0, 0, tmp);
    tmp.recycle();
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
    if (bmp == null)
      return false;

    result[1] = bmp.getWidth();
    result[2] = bmp.getHeight();

    if (bmp.getConfig() == null) {
      /* convert to a format compatible with OpenGL */
      Bitmap tmp = bmp.copy(Bitmap.Config.RGB_565, false);
      bmp.recycle();

      if (tmp == null)
        return false;

      bmp = tmp;
    }

    /* create and configure an OpenGL texture */

    gl.glGenTextures(1, result, 0);
    gl.glBindTexture(GL10.GL_TEXTURE_2D, result[0]);
    gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER,
                       GL10.GL_NEAREST);
    gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER,
                       GL10.GL_NEAREST);

    try {
      loadTexture(bmp);
    } catch (Exception e) {
      Log.e(TAG, "GLUtils error: " + e);
      return false;
    } finally {
      bmp.recycle();
    }

    /* done */

    return true;
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
