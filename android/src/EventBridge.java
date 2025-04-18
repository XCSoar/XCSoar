// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

class EventBridge {
  public static native void onKeyDown(int keyCode);
  public static native void onKeyUp(int keyCode);
  public static native void onMouseDown(int x, int y);
  public static native void onMouseUp(int x, int y);
  public static native void onMouseMove(int x, int y);
  public static native void onPointerDown();
  public static native void onPointerUp();
}
