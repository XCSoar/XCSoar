// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.Closeable;

/**
 * The Java interface of the C++ AndroidPort class.
 */
interface AndroidPort extends AndroidSensor {
  void setListener(PortListener listener);

  void setInputListener(InputListener listener);

  /**
   * @see Port::Drain()
   */
  boolean drain();

  int getBaudRate();
  boolean setBaudRate(int baud);

  /**
   * Write data to the port.  Execution blocks until at least one
   * bytes is written or an error occurs or until the timeout expires.
   *
   * @param data the data to be written
   * @param length the number of bytes to be written
   * @return the number of bytes that were written or -1 on error/timeout
   */
  int write(byte[] data, int length);
}
