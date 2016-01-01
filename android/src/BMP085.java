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
import ioio.lib.api.TwiMaster;
import ioio.lib.api.DigitalInput;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the BMP085 pressure sensor, connected via IOIO.
 *
 * @see http://www.bosch-sensortec.com/content/language1/html/3477.htm
 */
final class BMP085 extends Thread {
  interface Listener {
    /**
     * @param pressure the pressure [Pa]
     */
    void onBMP085Values(double temperature, int pressure);
    void onBMP085Error();
  };

  private static final String TAG = "XCSoar";

  static final byte BMP085_DEVICE_ID = 0x77;
  static final byte BMP085_CHIP_ID = 0x55;
  static final byte BMP085_CALIBRATION_DATA_START = (byte)0xaa;
  static final int BMP085_CALIBRATION_DATA_LENGTH = 11;
  static final byte BMP085_CHIP_ID_REG = (byte)0xd0;
  static final byte BMP085_CTRL_REG = (byte)0xf4;
  static final byte BMP085_TEMP_MEASUREMENT = 0x2e;
  static final byte BMP085_PRESSURE_MEASUREMENT = 0x34;
  static final byte BMP085_CONVERSION_REGISTER_MSB = (byte)0xf6;
  static final byte BMP085_CONVERSION_REGISTER_LSB = (byte)0xf7;
  static final byte BMP085_CONVERSION_REGISTER_XLSB = (byte)0xf8;
  static final int BMP085_TEMP_CONVERSION_TIME = 5;

  static final int SMD500_PARAM_MG = 3038;
  static final int SMD500_PARAM_MH = -7357;
  static final int SMD500_PARAM_MI = 3791;

  private static final byte[] requestChipID = new byte[] {
    BMP085_CHIP_ID_REG
  };

  private static final byte[] requestParameters = new byte[] {
    BMP085_CALIBRATION_DATA_START
  };

  private byte[] responseParameters = new byte[BMP085_CALIBRATION_DATA_LENGTH * 2];

  private static final byte[] readTemperature = new byte[] {
    BMP085_CTRL_REG, BMP085_TEMP_MEASUREMENT
  };

  private byte[] readPressure = new byte[] {
    BMP085_CTRL_REG, BMP085_PRESSURE_MEASUREMENT
  };

  private static final byte[] requestValue = new byte[] {
    BMP085_CONVERSION_REGISTER_MSB
  };

  private byte[] response = new byte[3];

  private TwiMaster twi;
  private DigitalInput eoc;
  private final int oversampling;
  private final Listener listener;

  /**
   * Pressure calibration coefficients.
   */
  private int ac1, ac2, ac3, ac4, b1, b2;

  /**
   * Temperature calibration coefficients.
   */
  private int ac5, ac6, mc, md;

  public BMP085(IOIO ioio, int twiNum, int eocPin, int _oversampling,
                Listener _listener)
    throws ConnectionLostException {
    super("BMP085");

    twi = ioio.openTwiMaster(twiNum, TwiMaster.Rate.RATE_1MHz, false);
    eoc = ioio.openDigitalInput(eocPin);

    oversampling = _oversampling;
    readPressure[1] += oversampling << 6;

    listener = _listener;

    start();
  }

  public void close() {
    if (twi != null)
      twi.close();

    if (eoc != null)
      eoc.close();

    interrupt();

    try {
      join();
    } catch (InterruptedException e) {
    }
  }

  private static int readU16BE(byte[] data, int offset) {
    return ((data[offset] & 0xff) << 8) | (data[offset + 1] & 0xff);
  }

  private static int readS16BE(byte[] data, int offset) {
    return (short)readU16BE(data, offset);
  }

  private static int readU24BE(byte[] data, int offset) {
    return ((data[offset] & 0xff) << 16) | ((data[offset + 1] & 0xff) << 8)
      | (data[offset + 2] & 0xff);
  }

  private boolean setup()
    throws ConnectionLostException, InterruptedException {
    /* is it a BMP085 sensor? */
    byte[] responseChipID = new byte[1];
    twi.writeRead(BMP085_DEVICE_ID, false,
                  requestChipID, requestChipID.length,
                  responseChipID, responseChipID.length);
    if (responseChipID[0] != BMP085_CHIP_ID) {
      Log.e(TAG, "Not a BMP085: "
            + Integer.toHexString(responseChipID[0] & 0xff));
      return false;
    }

    /* read the calibration coefficients */
    twi.writeRead(BMP085_DEVICE_ID, false,
                  requestParameters, requestParameters.length,
                  responseParameters, responseParameters.length);

    ac1 = readS16BE(responseParameters, 0);
    ac2 = readS16BE(responseParameters, 2);
    ac3 = readS16BE(responseParameters, 4);
    ac4 = readU16BE(responseParameters, 6);
    ac5 = readU16BE(responseParameters, 8);
    ac6 = readU16BE(responseParameters, 10);
    b1 = readS16BE(responseParameters, 12);
    b2 = readS16BE(responseParameters, 14);
    mc = readS16BE(responseParameters, 18);
    md = readS16BE(responseParameters, 20);

    return true;
  }

  private void readSensor(byte[] request, byte[] response, int responseLength)
    throws ConnectionLostException, InterruptedException {
    twi.writeRead(BMP085_DEVICE_ID, false,
                  request, request.length, response, 0);

    /* wait until the new value becomes ready */
    eoc.waitForValue(true);

    twi.writeRead(BMP085_DEVICE_ID, false,
                  requestValue, requestValue.length,
                  response, responseLength);
  }

  private int getB5(int ut) {
    int x1 = ((ut - ac6) * ac5) >> 15;
    int x2 = (mc << 11) / (x1 + md);
    return x1 + x2;
  }

  private int getB3(int b6) {
    int x1 = (((b6 * b6) >> 12) * b2) >> 11;
    int x2 = (ac2 * b6) >> 11;

    int x3 = x1 + x2;

    return (((ac1 * 4 + x3) << oversampling) + 2) >> 2;
  }

  private int getB4(int b6) {
    int x1 = (ac3 * b6) >> 13;
    int x2 = (b1 * ((b6 * b6) >> 12)) >> 16;
    int x3 = (x1 + x2 + 2) >> 2;
    return (ac4 * (x3 + 32768)) >> 15;
  }

  /**
   * @return pressure [Pa]
   */
  private int getPressure(int up, int param_b5) {
    int b6 = param_b5 - 4000;
    int b3 = getB3(b6);
    int b4 = getB4(b6);

    int b7 = ((up - b3) * (50000 >> oversampling));

    int pressure;
    if (b7 < 0x80000000)
      pressure = (b7 << 1) / b4;
    else
      pressure = (b7 / b4) << 1;

    int x1 = pressure >> 8;
    x1 *= x1;

    x1 = (x1 * SMD500_PARAM_MG) >> 16;
    int x2 = (pressure * SMD500_PARAM_MH) >> 16;
    pressure += (x1 + x2 + SMD500_PARAM_MI) >> 4;

    return pressure;
  }

  private void loop() throws ConnectionLostException, InterruptedException {
    readSensor(readTemperature, response, 2);
    int b5 = getB5(readU16BE(response, 0));
    double temperature_c = (double)((b5 + 8) >> 4) / 10.0;

    readSensor(readPressure, response, 3);
    int up = readU24BE(response, 0) >> (8 - oversampling);
    int pressure_pa = getPressure(up, b5);

    listener.onBMP085Values(temperature_c, pressure_pa);
  }

  @Override public void run() {
    try {
      if (!setup())
        return;

      while (true)
        loop();
    } catch (ConnectionLostException e) {
      Log.d(TAG, "BMP085.run() failed", e);
    } catch (IllegalStateException e) {
      Log.d(TAG, "BMP085.run() failed", e);
    } catch (InterruptedException e) {
    } finally {
      listener.onBMP085Error();
    }
  }
}
