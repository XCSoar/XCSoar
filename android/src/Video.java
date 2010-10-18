/* This source was imported to XCSoar from
   git://github.com/pelya/commandergenius.git */

package org.xcsoar;

import android.app.Activity;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.os.Build;

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
      int action = -1;
      if( event.getAction() == MotionEvent.ACTION_DOWN )
        action = 0;
      if( event.getAction() == MotionEvent.ACTION_UP )
        action = 1;
      if( event.getAction() == MotionEvent.ACTION_MOVE )
        action = 2;
      if ( action >= 0 )
        DemoGLSurfaceView.nativeMouse( (int)event.getX(), (int)event.getY(), action, 0,
                        (int)(event.getPressure() * 1000.0),
                        (int)(event.getSize() * 1000.0) );
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
            action = 0;
        if( event.getAction() == MotionEvent.ACTION_UP )
          action = 1;
        if( event.getAction() == MotionEvent.ACTION_MOVE )
          action = 2;
        if ( action >= 0 )
            DemoGLSurfaceView.nativeMouse( (int)event.getX(),
                                           (int)event.getY(),
                                           action,
                                           0,
                                           (int)(event.getPressure() * 1000.0),
                                           (int)(event.getSize() * 1000.0));
    }
  }
}

class DemoGLSurfaceView extends GLSurfaceView_SDL {
  public DemoGLSurfaceView(Activity context) {
    super(context);
    touchInput = DifferentTouchInput.getInstance();
    setEGLConfigChooser(Globals.NeedDepthBuffer);
    mRenderer = new DemoRenderer();
    setRenderer(mRenderer);
  }

  @Override public boolean onTouchEvent(final MotionEvent event)
  {
    touchInput.process(event);
    // Wait a bit, and try to synchronize to app framerate, or event thread will eat all CPU and we'll lose FPS
    synchronized (mRenderer) {
      try {
        mRenderer.wait(300L);
      } catch (InterruptedException e) { }
    }
    return true;
  };

  public void exitApp() {
    mRenderer.exitApp();
  };

  @Override public void onPause() {
    super.onPause();
    mRenderer.mPaused = true;
  };

  @Override public void onResume() {
    super.onResume();
    mRenderer.mPaused = false;
    if( mRenderer.mGlSurfaceCreated && ! mRenderer.mPaused )
      mRenderer.nativeGlContextRecreated();
  };

  @Override public boolean onKeyDown(int keyCode, final KeyEvent event) {
    nativeKey( keyCode, 1 );
    return true;
  }

  @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
    nativeKey( keyCode, 0 );
    return true;
  }

  DemoRenderer mRenderer;
  DifferentTouchInput touchInput = null;

  public static native void nativeMouse( int x, int y, int action, int pointerId, int pressure, int radius );
  public static native void nativeKey( int keyCode, int down );
}
