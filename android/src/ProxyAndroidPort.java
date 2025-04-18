// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.IOException;

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
    if (oldPort != null) {
      try {
        oldPort.close();
      } catch (IOException e) {
      }
    }

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
    if (port != null) {
      try {
        port.close();
      } catch (IOException e) {
      }
    }

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
