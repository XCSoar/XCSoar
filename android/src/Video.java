/* This source was imported to XCSoar from
   git://github.com/pelya/commandergenius.git */

package org.xcsoar;

import android.app.Activity;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.os.Build;

class EventBridge {
  public static native void onKeyDown(int keyCode);
  public static native void onKeyUp(int keyCode);
  public static native void onMouseDown(int x, int y);
  public static native void onMouseUp(int x, int y);
  public static native void onMouseMove(int x, int y);
};

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
    EventBridge.onKeyDown(keyCode);
    return true;
  }

  @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
    EventBridge.onKeyDown(keyCode);
    return true;
  }

  DemoRenderer mRenderer;
  DifferentTouchInput touchInput = null;
}
