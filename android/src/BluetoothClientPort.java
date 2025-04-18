// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
