// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.UUID;
import java.util.Collection;
import java.util.Set;
import java.util.LinkedList;
import java.io.IOException;
import android.util.Log;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;

/**
 * A utility class which wraps the Java API into an easier API for the
 * C++ code.
 */
final class BluetoothServerPort extends MultiPort
  implements Runnable, InputListener {

  private static final String TAG = "XCSoar";

  private BluetoothServerSocket serverSocket;
  private Collection<BluetoothSocket> sockets =
    new LinkedList<BluetoothSocket>();

  private final Thread thread = new Thread(this, "Bluetooth server");

  BluetoothServerPort(BluetoothAdapter adapter, UUID uuid)
    throws IOException {
    serverSocket = adapter.listenUsingRfcommWithServiceRecord("XCSoar", uuid);

    thread.start();
  }

  private synchronized BluetoothServerSocket stealServerSocket() {
    BluetoothServerSocket s = serverSocket;
    serverSocket = null;
    return s;
  }

  private void closeServerSocket() {
    BluetoothServerSocket s = stealServerSocket();
    if (s == null)
      return;

    try {
      s.close();
    } catch (IOException e) {
      Log.e(TAG, "Failed to close Bluetooth server socket", e);
    }

    stateChanged();
  }

  @Override public void run() {
    while (true) {
      try {
        /* copy serverSocket to a local variable, so we can compare it
           with null without risking a race condition with the thread
           calling close() */
        BluetoothServerSocket ss = serverSocket;
        if (ss == null)
          /* close() is being called by another thread, which now
             waits for this thread to quit */
          break;

        BluetoothSocket socket = ss.accept();
        Log.i(TAG, "Accepted Bluetooth connection from " +
              BluetoothHelper.getDisplayString(socket));
        BluetoothPort port = new BluetoothPort(socket);

        /* make writes non-blocking and potentially lossy, to avoid
           blocking when one of the peers doesn't receive quickly
           enough */
        port.setWriteTimeout(0);

        add(port);
      } catch (IOException e) {
        Log.e(TAG, "Bluetooth server socket has failed", e);
        closeServerSocket();
        error(e.getMessage());
        break;
      }
    }
  }

  @Override public void close() {
    closeServerSocket();

    /* ensure that run() has finished before calling
       MultiPort.close() */
    try {
      thread.join();
    } catch (InterruptedException e) {
    }

    super.close();
  }

  @Override public int getState() {
    if (serverSocket == null)
      return STATE_FAILED;

    return super.getState();
  }
}
