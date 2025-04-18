// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.InputStream;
import java.io.OutputStream;

/**
 * Abstract base class to aid the real implementations (Bluetooth,
 * IOIO).
 */
abstract class AbstractAndroidPort implements AndroidPort {
  private final String name;
  private PortListener portListener;
  private InputListener inputListener;
  private InputThread input;
  private OutputThread output;

  protected AbstractAndroidPort(String _name) {
    name = _name;
  }

  @Override public String toString() {
    return name;
  }

  private synchronized InputThread stealInput() {
    InputThread i = input;
    input = null;
    return i;
  }

  private synchronized OutputThread stealOutput() {
    OutputThread o = output;
    output = null;
    return o;
  }

  protected synchronized void set(InputStream _input, OutputStream _output) {
    input = new InputThread(name, inputListener, _input);
    output = new OutputThread(name, _output);
    output.setTimeout(5000);
    stateChanged();
  }

  protected void setWriteTimeout(int timeout_ms) {
    output.setTimeout(timeout_ms);
  }

  @Override public void setListener(PortListener _listener) {
    portListener = _listener;
  }

  @Override public void setInputListener(InputListener _listener) {
    inputListener = _listener;
    if (input != null)
      input.setListener(_listener);
  }

  @Override public void close() {
    InputThread i = stealInput();
    if (i != null)
      i.close();

    OutputThread o = stealOutput();
    if (o != null)
      o.close();

    stateChanged();
  }

  @Override public int getState() {
    InputThread i = input;
    return i != null && i.isValid()
      ? STATE_READY
      : STATE_FAILED;
  }

  @Override public final boolean drain() {
    OutputThread o = output;
    return o != null && o.drain();
  }

  @Override public int write(byte[] data, int length) {
    OutputThread o = output;
    return o != null
      ? o.write(data, length)
      : -1;
  }

  protected final void stateChanged() {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portStateChanged();
  }

  protected final void submitError(String msg) {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portError(msg);
  }
}
