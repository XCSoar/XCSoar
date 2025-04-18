// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * An #InputListener implementation that passes method calls to native
 * code.
 */
final class NativeInputListener implements InputListener {
  /**
   * A native pointer.
   */
  private final long ptr;

  NativeInputListener(long _ptr) {
    ptr = _ptr;
  }

  @Override public native void dataReceived(byte[] data, int length);
}
