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

/**
 * A an abstract base class for #AndroidPort implementations that act
 * as a proxy for another #AndroidPort instance.
 */
abstract class ProxyAndroidPort implements AndroidPort {
  private volatile AndroidPort port;
  private volatile PortListener portListener;
  private volatile InputListener inputListener;

  protected void setPort(AndroidPort _port) {
    AndroidPort oldPort = this.port;
    port = _port;
    if (oldPort != null)
      oldPort.close();

    if (port != null)
      port.setInputListener(inputListener);

    stateChanged();
  }

  @Override public String toString() {
    AndroidPort port = this.port;
    return port != null
      ? port.toString()
      : super.toString();
  }

  @Override public void setListener(PortListener _listener) {
    portListener = _listener;

    AndroidPort port = this.port;
    if (port != null)
      port.setListener(_listener);
  }

  @Override public void setInputListener(InputListener _listener) {
    inputListener = _listener;

    AndroidPort port = this.port;
    if (port != null)
      port.setInputListener(_listener);
  }

  @Override public void close() {
    AndroidPort port = this.port;
    this.port = null;
    if (port != null)
      port.close();

    stateChanged();
  }

  @Override public int getState() {
    AndroidPort port = this.port;
    return port != null
      ? port.getState()
      : STATE_FAILED;
  }

  @Override public boolean drain() {
    AndroidPort port = this.port;
    return port != null && port.drain();
  }

  @Override public int getBaudRate() {
    AndroidPort port = this.port;
    return port != null ? port.getBaudRate() : 0;
  }

  @Override public boolean setBaudRate(int baud) {
    AndroidPort port = this.port;
    return port != null && port.setBaudRate(baud);
  }

  @Override public int write(byte[] data, int length) {
    AndroidPort port = this.port;
    return port != null
      ? port.write(data, length)
      : 0;
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
