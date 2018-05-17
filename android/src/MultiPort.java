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
        port.close();
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

  @Override public synchronized void close() {
    error = true;

    for (AndroidPort port : ports)
      port.close();

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
        port.close();
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
