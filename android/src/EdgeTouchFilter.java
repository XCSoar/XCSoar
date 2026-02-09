// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.Context;
import android.graphics.Insets;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Build;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.WindowInsets;
import android.view.WindowManager;

/**
 * Filters touch events to reject system edge gestures (back swipe,
 * status bar pull-down, etc.) that would otherwise be forwarded to
 * XCSoar's native input handling.
 *
 * Touches that start inside a system gesture inset zone and move in
 * the expected OS gesture direction are silently dropped; all other
 * touches are forwarded via {@link EventBridge}.
 */
class EdgeTouchFilter implements View.OnApplyWindowInsetsListener {
  /**
   * System touch-slop threshold for distinguishing taps from swipes.
   */
  private int touchSlop = 0;

  /**
   * System gesture inset values for detecting edge swipes.
   * Stored as individual int fields to avoid referencing the
   * Insets class (API 29+) in field declarations.
   */
  private int gestureInsetLeft = 0;
  private int gestureInsetRight = 0;
  private int gestureInsetTop = 0;
  private int gestureInsetBottom = 0;

  private int screenWidth = 0;
  private int screenHeight = 0;

  /** Per-sequence tracking state. */
  private int edgeTouchFlags = 0;
  private float edgeTouchStartX = 0;
  private float edgeTouchStartY = 0;
  private boolean edgeTouchRejected = false;
  private boolean edgeDownForwarded = false;

  /* ---- WindowInsets callback ---- */

  @Override
  public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
    if (Build.VERSION.SDK_INT >= 30) {
      /* Modern API; getSystemGestureInsets() is deprecated from 30 */
      Insets gi = insets.getInsets(WindowInsets.Type.systemGestures());
      gestureInsetLeft = gi.left;
      gestureInsetRight = gi.right;
      gestureInsetTop = gi.top;
      gestureInsetBottom = gi.bottom;
    } else if (Build.VERSION.SDK_INT >= 29) {
      Insets gi = insets.getSystemGestureInsets();
      gestureInsetLeft = gi.left;
      gestureInsetRight = gi.right;
      gestureInsetTop = gi.top;
      gestureInsetBottom = gi.bottom;
    }

    /* On devices without gesture navigation the system reports
       zero left/right insets, but XCSoar draws edge-to-edge so
       accidental edge touches are still likely (especially in
       flight with gloves).  Apply a minimum dead zone matching
       the standard Android back-gesture width (~24dp). */
    final float density = v.getResources().getDisplayMetrics().density;
    final int minEdge = (int)(24 * density + 0.5f);
    if (gestureInsetLeft < minEdge)
      gestureInsetLeft = minEdge;
    if (gestureInsetRight < minEdge)
      gestureInsetRight = minEdge;

    if (Build.VERSION.SDK_INT >= 30) {
      WindowManager wm =
        v.getContext().getSystemService(WindowManager.class);
      Rect bounds = wm.getCurrentWindowMetrics().getBounds();
      screenWidth = bounds.width();
      screenHeight = bounds.height();
    } else {
      WindowManager wm = (WindowManager) v.getContext()
        .getSystemService(Context.WINDOW_SERVICE);
      Display display = wm.getDefaultDisplay();
      Point size = new Point();
      display.getRealSize(size);
      screenWidth = size.x;
      screenHeight = size.y;
    }

    touchSlop =
      ViewConfiguration.get(v.getContext()).getScaledTouchSlop();

    return insets;
  }

  /* ---- Touch event filtering ---- */

  /**
   * Process a touch event.  Edge gestures are silently rejected;
   * all other events are forwarded to {@link EventBridge}.
   *
   * @param event   the raw MotionEvent
   * @param x       view-relative X coordinate (after offset correction)
   * @param y       view-relative Y coordinate (after offset correction)
   */
  void onTouchEvent(MotionEvent event, float x, float y) {
    final int finalX = (int)x;
    final int finalY = (int)y;

    switch (event.getActionMasked()) {
    case MotionEvent.ACTION_DOWN:
      if (screenHeight > 0 && screenWidth > 0 && touchSlop > 0) {
        /* Reset edge touch tracking state */
        edgeTouchStartX = x;
        edgeTouchStartY = y;
        edgeTouchRejected = false;
        edgeDownForwarded = true;
        edgeTouchFlags = 0;

        final float rawX = event.getRawX();
        final float rawY = event.getRawY();

        if (gestureInsetLeft > 0 && rawX < gestureInsetLeft)
          edgeTouchFlags |= MotionEvent.EDGE_LEFT;
        if (gestureInsetRight > 0 &&
            rawX > screenWidth - gestureInsetRight)
          edgeTouchFlags |= MotionEvent.EDGE_RIGHT;
        if (gestureInsetTop > 0 && rawY < gestureInsetTop)
          edgeTouchFlags |= MotionEvent.EDGE_TOP;
        if (gestureInsetBottom > 0 &&
            rawY > screenHeight - gestureInsetBottom)
          edgeTouchFlags |= MotionEvent.EDGE_BOTTOM;
      }
      /* Always forward ACTION_DOWN to allow focus changes */
      EventBridge.onMouseDown(finalX, finalY);
      break;

    case MotionEvent.ACTION_UP:
      if (!edgeTouchRejected)
        EventBridge.onMouseUp(finalX, finalY);

      edgeTouchFlags = 0;
      edgeTouchRejected = false;
      edgeDownForwarded = false;
      break;

    case MotionEvent.ACTION_MOVE:
      if (edgeTouchRejected)
        break;

      if (edgeTouchFlags != 0) {
        float dx = x - edgeTouchStartX;
        float dy = y - edgeTouchStartY;

        if (isOsEdgeGesture(dx, dy)) {
          edgeTouchRejected = true;
          if (edgeDownForwarded)
            EventBridge.onMouseCancel();
          break;
        }
      }

      EventBridge.onMouseMove(finalX, finalY);
      break;

    case MotionEvent.ACTION_POINTER_DOWN:
      if (!edgeTouchRejected)
        EventBridge.onPointerDown();
      break;

    case MotionEvent.ACTION_POINTER_UP:
      if (!edgeTouchRejected)
        EventBridge.onPointerUp();
      break;

    case MotionEvent.ACTION_CANCEL:
      EventBridge.onMouseCancel();
      edgeTouchFlags = 0;
      edgeTouchRejected = false;
      edgeDownForwarded = false;
      break;
    }
  }

  /**
   * Check if the current touch movement matches an OS edge gesture.
   */
  private boolean isOsEdgeGesture(float dx, float dy) {
    if (edgeTouchFlags == 0)
      return false;

    /* Left edge: horizontal swipe to the right */
    if ((edgeTouchFlags & MotionEvent.EDGE_LEFT) != 0 &&
        dx > touchSlop && Math.abs(dx) > Math.abs(dy))
      return true;

    /* Right edge: horizontal swipe to the left */
    if ((edgeTouchFlags & MotionEvent.EDGE_RIGHT) != 0 &&
        dx < -touchSlop && Math.abs(dx) > Math.abs(dy))
      return true;

    /* Top edge: vertical swipe downward */
    if ((edgeTouchFlags & MotionEvent.EDGE_TOP) != 0 &&
        dy > touchSlop && Math.abs(dy) > Math.abs(dx))
      return true;

    /* Bottom edge: vertical swipe upward */
    if ((edgeTouchFlags & MotionEvent.EDGE_BOTTOM) != 0 &&
        dy < -touchSlop && Math.abs(dy) > Math.abs(dx))
      return true;

    return false;
  }
}
