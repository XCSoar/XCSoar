// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the BMP085 pressure sensor, connected via IOIO.
 */
final class GlueBMP085 implements AndroidSensor, IOIOConnectionListener {
  private IOIOConnectionHolder holder;
  private final int twiNum, eocPin, oversampling;
  private final SensorListener listener;

  private BMP085 instance;
  private int state = STATE_LIMBO;

  GlueBMP085(IOIOConnectionHolder _holder,
             int _twiNum, int _eocPin, int _oversampling,
             SensorListener _listener) {
    twiNum = _twiNum;
    eocPin = _eocPin;
    oversampling = _oversampling;
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
    instance = new BMP085(ioio, twiNum, eocPin, oversampling, listener);
    state = STATE_READY;
    listener.onSensorStateChanged();
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
