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

import android.util.Log;
import ioio.lib.api.IOIO;
import ioio.lib.api.Uart;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * ID = 0, pins TX=3, RX=4
 * ID = 1, pins TX=5, RX=6
 * ID = 2, pins TX=10 RX=11
 * ID = 3, pins TX=12 RX=13
 *
 */
final class GlueIOIOPort extends IOIOPort implements IOIOConnectionListener {
  private static final String TAG = "XCSoar";

  private IOIOConnectionHolder holder;

  /**
   * Is the #IOIOConnectionHolder currently connected to an IOIO
   * board?
   */
  private boolean connected;

  private boolean constructing;

  private final int inPin;
  private final int outPin;
  private int baudrate = 0;
  private int ID;

  GlueIOIOPort(IOIOConnectionHolder _holder, int ID_, int _baudrate) {
    super("IOIO UART " + ID_);

    ID = ID_;
    baudrate = _baudrate;

    switch (ID) {
    case 0:
    case 1:
      inPin = (ID * 2) + 4;
      outPin = inPin - 1;
      break;
    case 2:
    case 3:
      inPin = (ID * 2) + 7;
      outPin = inPin - 1;
      break;
    default:
      throw new IllegalArgumentException();
    }

    holder = _holder;
    _holder.addListener(this);
  }

  @Override public void onIOIOConnect(IOIO ioio)
    throws ConnectionLostException, InterruptedException {

    try {
      synchronized(this) {
        connected = true;
        constructing = true;
      }

      stateChanged();

      Uart uart;
      try {
        uart = ioio.openUart(inPin, outPin, baudrate, Uart.Parity.NONE,
                             Uart.StopBits.ONE);
      } catch (IllegalArgumentException e) {
        Log.w(TAG, "IOIO.openUart() failed", e);
        return;
      }

      set(uart);
    } finally {
      synchronized(this) {
        constructing = false;
        notifyAll();
      }

      stateChanged();
    }
  }

  @Override public void onIOIODisconnect(IOIO ioio) {
    connected = false;
    stateChanged();

    super.close();
  }

  @Override public void close() {
    IOIOConnectionHolder holder;
    synchronized(this) {
      holder = this.holder;
      this.holder = null;
    }

    if (holder != null)
      holder.removeListener(this);
  }

  @Override public int getState() {
    boolean ready;
    synchronized(this) {
      ready = connected && !constructing;
    }

    return ready
      ? super.getState()
      : STATE_LIMBO;
  }

  @Override public int getBaudRate() {
    return baudrate;
  }

  @Override public boolean setBaudRate(int _baudrate) {
    if (_baudrate == baudrate)
      return true;

    IOIOConnectionHolder holder = this.holder;
    if (holder == null)
      /* this port was already closed */
      return false;

    final boolean wasConnected = connected;

    baudrate = _baudrate;
    holder.cycleListener(this);

    if (wasConnected) {
      try {
        /* wait until the port has been reconnected after a baud rate
           change; onIOIOConnect() will be called in another thread, and
           any attempt to do I/O before onIOIOConnect() has finished is
           doomed to fail */
        synchronized(this) {
          wait(200);
        }
      } catch (InterruptedException e) {
      }
    }

    return true;
  }
}
