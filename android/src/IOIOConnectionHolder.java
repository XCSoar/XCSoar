// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * An object that manages the connection to a IOIO board.
 */
interface IOIOConnectionHolder {
  void addListener(IOIOConnectionListener l);
  void removeListener(IOIOConnectionListener l);

  /**
   * The specified listener wants to be invoked again, as if the
   * connection had been lost and re-established.  This is a no-op if
   * no connection exists currently.
   */
  void cycleListener(IOIOConnectionListener l);
}
