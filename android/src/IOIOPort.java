// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import ioio.lib.api.Uart;

/**
 * Wrapper for an IOIO UART in an AndroidPort interface.
 */
abstract class IOIOPort extends AbstractAndroidPort {
  private Uart uart;

  IOIOPort(String name) {
    super(name);
  }

  protected boolean isSet() {
    return uart != null;
  }

  protected void set(Uart _uart) {
    uart = _uart;
    super.set(_uart.getInputStream(), _uart.getOutputStream());
  }

  @Override public void close() {
    super.close();

    Uart uart = this.uart;
    this.uart = null;
    if (uart != null)
      uart.close();
  }
}
