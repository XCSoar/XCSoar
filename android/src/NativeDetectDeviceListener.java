// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * An #DetectDeviceListener implementation that passes method calls to
 * native code.
 */
final class NativeDetectDeviceListener implements DetectDeviceListener {
  /**
   * A native pointer.
   */
  private final long ptr;

  NativeDetectDeviceListener(long _ptr) {
    ptr = _ptr;
  }

  @Override
  public native void onDeviceDetected(int type, String address,
                                      String name, long features);
}
