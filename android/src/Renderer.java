/* This source was imported to XCSoar from
   git://github.com/pelya/commandergenius.git */

package org.xcsoar;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.util.Log;

class DemoRenderer extends GLSurfaceView_SDL.Renderer {
    private static final String TAG = "XCSoar";
    public static boolean loaded = false;

    static {
        try {
            System.loadLibrary("sdl-1.2");
            System.loadLibrary("sdl_gfx");
            System.loadLibrary("sdl_ttf");
            System.loadLibrary("application");
            loaded = true;
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, e.getMessage());
        }

        Settings.nativeSetMouseUsed();
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        mGlSurfaceCreated = true;
        if( mGlSurfaceCreated && ! mPaused && ! mFirstTimeStart )
            nativeGlContextRecreated();
        mFirstTimeStart = false;
    }

    public void onSurfaceChanged(GL10 gl, int w, int h) {
        nativeResize(w, h);
    }

    public void onSurfaceDestroyed() {
        mGlSurfaceCreated = false;
        mGlContextLost = true;
        nativeGlContextLost();
    };

    public void onDrawFrame(GL10 gl) {
        nativeInitJavaCallbacks();

        // Make main thread priority lower so audio thread won't get underrun
        // Thread.currentThread().setPriority((Thread.currentThread().getPriority() + Thread.MIN_PRIORITY)/2);

        mGlContextLost = false;
        nativeInit(); // Calls main() and never returns, hehe - we'll call eglSwapBuffers() from native code
        System.exit(0); // The main() returns here - I don't bother with deinit stuff, just terminate process
    }

    public int swapBuffers() // Called from native code
    {
        synchronized (this) {
            this.notify();
        }
        if( ! super.SwapBuffers() && Globals.NonBlockingSwapBuffers )
            return 0;
        if(mGlContextLost) {
            mGlContextLost = false;
        }
        return 1;
    }

    public void exitApp() {
        nativeDone();
    };

    private native void nativeInitJavaCallbacks();
    private native void nativeInit();
    private native void nativeResize(int w, int h);
    private native void nativeDone();
    private native void nativeGlContextLost();
    public native void nativeGlContextRecreated();

    private EGL10 mEgl = null;
    private EGLDisplay mEglDisplay = null;
    private EGLSurface mEglSurface = null;
    private EGLContext mEglContext = null;
    private boolean mGlContextLost = false;
    public boolean mGlSurfaceCreated = false;
    public boolean mPaused = false;
    private boolean mFirstTimeStart = true;
}
