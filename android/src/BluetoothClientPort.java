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
import android.util.Log;
import android.bluetooth.BluetoothSocket;

/**
 * An #AndroidPort implementation that initiates a Bluetooth RFCOMM
 * connection.
 */
class BluetoothClientPort extends ProxyAndroidPort implements Runnable {
  private static final String TAG = "XCSoar";

  private BluetoothSocket socket;

  private Thread thread;

  BluetoothClientPort(BluetoothSocket _socket)
    throws IOException {
    socket = _socket;

    thread = new Thread(this, toString());
    thread.start();
  }

  @Override public String toString() {
    BluetoothSocket socket = this.socket;
    return socket != null
      ? "Bluetooth " + BluetoothHelper.getDisplayString(socket)
      : super.toString();
  }

  @Override public void close() {
    BluetoothSocket socket = this.socket;
    if (socket != null) {
      try {
        socket.close();
      } catch (IOException e) {
        Log.w(TAG, "Failed to close BluetoothSocket", e);
      }
    }

    /* ensure that run() has finished before calling
       ProxyAndroidPort.close() */
    Thread thread = this.thread;
    if (thread != null) {
      try {
        thread.join();
      } catch (InterruptedException e) {
      }
    }

    super.close();
  }

  @Override public int getState() {
    return socket != null
      ? STATE_LIMBO
      : super.getState();
  }

  @Override public void run() {
    try {
      BluetoothSocket socket = this.socket;
      if (socket != null) {
        socket.connect();
        this.socket = null;
        setPort(new BluetoothPort(socket));
      }
    } catch (Exception e) {
      Log.e(TAG, "Failed to connect to Bluetooth", e);
      error(e.getMessage());
    } finally {
      socket = null;
      thread = null;
      stateChanged();
    }
  }
}
