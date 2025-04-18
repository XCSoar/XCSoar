// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * An #PortListener implementation that passes method calls to native
 * code.
 */
final class NativePortListener implements PortListener {
  /**
   * A native pointer.
   */
  private final long ptr;

  NativePortListener(long _ptr) {
    ptr = _ptr;
  }

  @Override public native void portStateChanged();
  @Override public native void portError(String msg);
}
