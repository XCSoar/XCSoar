// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the Nintendo Nunchuck, connected via IOIO.
 */
final class GlueNunchuck implements AndroidSensor, IOIOConnectionListener {
  private IOIOConnectionHolder holder;
  private final int twiNum, sample_rate;
  private final SensorListener listener;

  private Nunchuck instance;
  private int state = STATE_LIMBO;

  GlueNunchuck(IOIOConnectionHolder _holder,
               int _twiNum, int _sample_rate,
               SensorListener _listener) {
    twiNum = _twiNum;
    sample_rate = _sample_rate;
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
      instance = new Nunchuck(ioio, twiNum, sample_rate, listener);
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
