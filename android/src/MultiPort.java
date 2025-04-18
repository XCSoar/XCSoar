// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.IOException;
import java.util.Collection;
import java.util.LinkedList;
import java.util.Iterator;
import android.util.Log;

/**
 * An #AndroidPort implementation that combines multiple #AndroidPort
 * objects.
 */
class MultiPort implements AndroidPort, InputListener {
  private PortListener portListener;
  private InputListener inputListener;

  private static final String TAG = "XCSoar";

  private Collection<AndroidPort> ports = new LinkedList<AndroidPort>();
  private boolean error = false;

  private synchronized int checkValid() {
    boolean ready = false, limbo = false;

    for (Iterator<AndroidPort> i = ports.iterator(); i.hasNext();) {
      AndroidPort port = i.next();

      switch (port.getState()) {
      case STATE_READY:
        ready = true;
        break;

      case STATE_FAILED:
        Log.i(TAG, "Bluetooth disconnect from " + port);

        i.remove();
        try {
          port.close();
        } catch (IOException e) {
        }
        error = true;
        break;

      case STATE_LIMBO:
        limbo = true;
        break;
      }
    }

    if (ready) {
      error = false;
      return STATE_READY;
    } else if (limbo || !error) {
      error = false;
      return STATE_LIMBO;
    } else
      return STATE_FAILED;
  }

  public synchronized void add(AndroidPort port) {
    error = false;
    checkValid();

    ports.add(port);
    port.setListener(portListener);
    port.setInputListener(this);
  }

  @Override public void setListener(PortListener _listener) {
    portListener = _listener;
  }

  @Override public void setInputListener(InputListener _listener) {
    inputListener = _listener;
  }

  @Override
  public synchronized void close() {
    error = true;

    for (AndroidPort port : ports) {
      try {
        port.close();
      } catch (IOException e) {
      }
    }

    ports.clear();
  }

  @Override public int getState() {
    return checkValid();
  }

  @Override public boolean drain() {
    // XXX not implemented
    return true;
  }

  @Override public synchronized int getBaudRate() {
    // XXX not implemented
    return 19200;
  }

  @Override public boolean setBaudRate(int baud) {
    // XXX not implemented
    return true;
  }

  @Override public synchronized int write(byte[] data, int length) {
    int result = -1;

    for (Iterator<AndroidPort> i = ports.iterator(); i.hasNext();) {
      AndroidPort port = i.next();
      int nbytes = port.write(data, length);
      if (nbytes < 0 && port.getState() == STATE_FAILED) {
        error = true;
        i.remove();

        try {
          port.close();
        } catch (IOException e) {
        }
      } else if (nbytes > result)
        result = nbytes;
    }

    return result;
  }

  @Override public void dataReceived(byte[] data, int length) {
    InputListener l = inputListener;
    if (l != null)
      l.dataReceived(data, length);
  }

  protected void stateChanged() {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portStateChanged();
  }

  protected void error(String msg) {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portError(msg);
  }
}
