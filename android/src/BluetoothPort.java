// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.IOException;
import android.bluetooth.BluetoothSocket;

/**
 * An #AndroidPort implementation for a Bluetooth RFCOMM connection.
 */
class BluetoothPort extends AbstractAndroidPort {
  private BluetoothSocket socket;

  BluetoothPort(BluetoothSocket _socket)
    throws IOException {
    super("Bluetooth " + BluetoothHelper.getDisplayString(_socket));

    socket = _socket;

    super.set(socket.getInputStream(), socket.getOutputStream());
  }

  public void close() {
    super.close();

    try {
      socket.close();
    } catch (IOException e) {
      /* ignore... what else should we do if closing the socket
         fails? */
    }
  }

  public int getBaudRate() {
    return 0;
  }

  public boolean setBaudRate(int baudRate) {
    return true;
  }
}
