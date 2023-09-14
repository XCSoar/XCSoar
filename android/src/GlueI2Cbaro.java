// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the I2C pressure sensors, connected via IOIO.
 */
final class GlueI2Cbaro implements AndroidSensor, IOIOConnectionListener {
  private IOIOConnectionHolder holder;
  private final int index;
  private final int twiNum, i2c_addr, sample_rate, flags;
  private final SensorListener listener;

  private I2Cbaro instance;
  private int state = STATE_LIMBO;

  GlueI2Cbaro(IOIOConnectionHolder _holder, int index,
              int _twiNum, int _i2c_addr, int _sample_rate, int _flags,
              SensorListener _listener) {
    this.index = index;
    twiNum = _twiNum;
    i2c_addr = _i2c_addr;
    sample_rate = _sample_rate;
    flags = _flags;
    listener = _listener;

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
      instance = new I2Cbaro(ioio, index, twiNum, i2c_addr, sample_rate, flags,
                             listener);
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
