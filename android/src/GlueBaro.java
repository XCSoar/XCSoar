/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the I2C pressure sensors, connected via IOIO.
 */
final class GlueBaro implements IOIOConnectionListener {
  private IOIOConnectionHolder holder;
  private final int type, bus, addr, sample_rate, flags;
  private final Baro.Listener listener;

  private Baro instance;

  GlueBaro(IOIOConnectionHolder _holder, int _type, 
              int _bus, int _addr, int _sample_rate, int _flags,
             Baro.Listener _listener) {
    type = _type;
    bus = _bus;
    addr = _addr;
    sample_rate = _sample_rate;
    flags = _flags;
    listener = _listener;

    holder = _holder;
    _holder.addListener(this);
  }

  public void close() {
    IOIOConnectionHolder holder;
    synchronized(this) {
      holder = this.holder;
      this.holder = null;
    }

    if (holder != null)
      holder.removeListener(this);
  }

  @Override public void onIOIOConnect(IOIO ioio)
    throws ConnectionLostException, InterruptedException {

	switch(type) {
    case 1: instance = new BaroAnalog(ioio, bus, sample_rate, flags, listener);
      break;
    case 2: instance = new BaroBMP085(ioio, bus, addr, sample_rate, flags, listener);
      break;
    case 3: instance = new BaroMS5611(ioio, bus, addr, sample_rate, flags, listener);
      break;
    default: instance = null;
    }
  }

  @Override public void onIOIODisconnect(IOIO ioio) {
    if (instance == null)
      return;

    instance.close();
    instance = null;
  }
}
