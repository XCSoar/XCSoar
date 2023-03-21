// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;

/*
 * A driver for voltage measurement on the IOIO board.
 */
final class GlueVoltage implements AndroidSensor, IOIOConnectionListener {
  private IOIOConnectionHolder holder;
  private final SensorListener listener;
  private final int sample_rate;
  private Voltage instance;
  private int state = STATE_LIMBO;

  GlueVoltage(IOIOConnectionHolder _holder, int _sample_rate,
              SensorListener _listener) {
    listener = _listener;
    sample_rate = _sample_rate;
    holder = _holder;
    _holder.addListener(this);
  }

  @Override
  public void close() {
    IOIOConnectionHolder holder;
    synchronized(this) {
      holder = this.holder;
      this.holder = null;
    }

    if (holder != null)
      holder.removeListener(this);
  }

  @Override
  public int getState() {
    return state;
  }

  @Override public void onIOIOConnect(IOIO ioio)
    throws ConnectionLostException, InterruptedException {
    try {
      instance = new Voltage(ioio, sample_rate, listener);
      state = STATE_READY;
      listener.onSensorStateChanged();
    } catch (Exception e) {
      state = STATE_FAILED;
      listener.onSensorError(e.getMessage());
    }
  }

  @Override public void onIOIODisconnect(IOIO ioio) {
    if (instance == null)
      return;

    instance.close();
    instance = null;
    state = STATE_LIMBO;
    listener.onSensorStateChanged();
  }
}
