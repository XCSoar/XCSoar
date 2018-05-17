/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
