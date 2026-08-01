// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

class EventBridge {
  public static native void onKeyDown(int keyCode);
  public static native void onKeyUp(int keyCode);
  public static native void onMouseDown(int x, int y);
  public static native void onMouseUp(int x, int y);
  public static native void onMouseMove(int x, int y);
  public static native void onMouseCancel();

  /**
   * A second pointer was pressed.  Coordinates are view-relative
   * pixels for the first two active pointers.
   */
  public static native void onPointerDown(int x1, int y1, int x2, int y2);

  /**
   * Two (or more) pointers are moving.  Coordinates are view-relative
   * pixels for the first two active pointers.
   */
  public static native void onPointerMove(int x1, int y1, int x2, int y2);

  public static native void onPointerUp();
}
