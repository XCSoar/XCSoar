/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the MS5611 pressure sensor, connected via IOIO.
 * @see http://http://www.meas-spec.com/downloads/ms5611-01ba01.pdf
 *
 * I use a 100 KHz I2C clock because of radio interference reasons,
 *   the MS5611 is not in the same enclosure as the IOIO board.
 *
 */
final class MS5611 extends Thread {
  interface Listener {
    /**
     * @param pressure the pressure [Pa]
     */
    void onMS5611Values(int pressure);
    void onMS5611Error();
  };

  private static final String TAG = "XCSoar";

  static final byte MS5611_DEVICE_ID = 0x76;
  static final byte CMD_RESET =              0x1E; /* ADC reset command */
  static final byte CMD_ADC_READ =           0x00; /* ADC read command */
  static final byte CMD_ADC_PRESS =          0x40; /* ADC D1 conversion */
  static final byte CMD_ADC_TEMP =           0x50; /* ADC D2 conversion */
  static final byte CMD_ADC_256 =            0x00; /* ADC OSR=256 */
  static final byte CMD_ADC_512 =            0x02; /* ADC OSR=512 */
  static final byte CMD_ADC_1024 =           0x04; /* ADC OSR=1024 */
  static final byte CMD_ADC_2048 =           0x06; /* ADC OSR=2048 */
  static final byte CMD_ADC_4096 =           0x08; /* ADC OSR=4096 */
  static final int  CMD_PROM_RD =            0xA0; /* Prom read command */

  static final byte oversampling = CMD_ADC_4096;	// I see no reason to use anything else.

  private int sleeptime = 7; // sleep at least 7 ms (IOIO command needs ~4ms, conversion time max. 9 ms
  private TwiMaster h_twi;
  private final Listener listener;

  private static byte[] requestCaldata = new byte[1];

  private static final byte[] requestReset = new byte[] {
    CMD_RESET
  };

  private static final byte[] requestTemp = new byte[] {
    CMD_ADC_TEMP + oversampling
  };

  private static final byte[] requestPress = new byte[] {
    CMD_ADC_PRESS + oversampling
  };

  private static final byte[] requestValue = new byte[] {
    CMD_ADC_READ
  };

  private byte[] dummy = new byte [] { 0 };

  private byte[] response = new byte[3];

  /**
   * Pressure calibration coefficients, C1s and C5s are left shifted from C1 and C5.
   */
  private long C1s=0, C2s=0, C3=0, C4=0;

  /**
   * Temperature calibration coefficients.
   */
  private int C5s=0; private long C6=0;

  public MS5611(IOIO ioio, int twiNum, int _sleeptime,
                Listener _listener)
    throws ConnectionLostException {
    super("MS5611");

    h_twi = ioio.openTwiMaster(twiNum, TwiMaster.Rate.RATE_100KHz, false);
    if (_sleeptime > sleeptime)
      sleeptime = _sleeptime;
    listener = _listener;

    start();
  }

  public void close() {
    if (h_twi != null)
      h_twi.close();

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

  private void reset()
    throws ConnectionLostException, InterruptedException {

    h_twi.writeRead(MS5611_DEVICE_ID, false, requestReset, requestReset.length, dummy, 0);
    sleep(5);
  }

  private boolean setup()
    throws ConnectionLostException, InterruptedException {

    int[] prom = new int [8];

    reset();

    /* Untill Checksum is implemented: too many zero results from EEPROM means no barometer present */
    int zeros = 0;

    /* Read Caldata from EEPROM */
    for (int i = 0; i<8; i++) {
      response[0]=0;response[1]=0;response[2]=0;
      requestCaldata[0] = (byte)(CMD_PROM_RD + 2*i);
      h_twi.writeRead(MS5611_DEVICE_ID, false, requestCaldata, requestCaldata.length, dummy, 0);
      h_twi.writeRead(MS5611_DEVICE_ID, false, dummy, 0, response, 2);
      prom[i] = readU16BE(response, 0);
      if (prom[i] == 0) zeros++;
    }

    // TODO: checksum
    if (zeros > 1)
        return false;

    C1s = ((long)prom[1]) * 32768L;
    C2s = ((long)prom[2]) * 65536L;
    C3  = prom[3];
    C4  = prom[4];
    C5s = prom[5] * 256;
    C6  = prom[6];

    return true;
  }

  private int readSensor()
    throws ConnectionLostException, InterruptedException {
    h_twi.writeRead(MS5611_DEVICE_ID, false, requestValue, requestValue.length, dummy, 0);
    h_twi.writeRead(MS5611_DEVICE_ID, false, dummy, 0 , response, 3);
    return readU24BE(response, 0);
  }

  static int loop_count = 0;
  static int TEMP;
  static long dT;
  private void loop() throws ConnectionLostException, InterruptedException {

    /* for some reason it does not work using shifts on long variables */

    int D1, P=0;

    /* Start temp conversion but only every 10th loop. */
    if (loop_count % 10 == 0) {
	h_twi.writeRead(MS5611_DEVICE_ID, false, requestTemp, requestTemp.length, dummy, 0);
        sleep(sleeptime);
        /* Read temp value and compute temperature */
        dT = readSensor() - C5s;
        TEMP = (int)(2000L + ((dT * C6) >> 23));
    }

    /* Start pressure conversion. */
    h_twi.writeRead(MS5611_DEVICE_ID, false, requestPress, requestPress.length, dummy, 0);
    sleep(sleeptime);

    /* Read pressure value */
    D1 = readSensor();

    long OFF = C2s + (C4 * dT) / 128L;
    long SENS = C1s + (C3 * dT) / 256L;

    int off2 = 0, sens2 = 0;

    if (TEMP < 2000) {
        // Only pressure 2nd order effects implemented
        int d2 = TEMP - 2000; d2 = d2 * d2;
        off2 = (5 * d2) / 2;
        sens2 = (5 * d2) / 4;
        if (TEMP < -1500) {
            d2 = TEMP + 1500; d2 = d2 * d2;
            off2 += (7 * d2);
            sens2 += (11 * d2) / 2;
        }
    }
    OFF -= off2;
    SENS -= sens2;
    P = (int)(((D1 * SENS) / 2097152L /* 2^21 */ - OFF) / 32768L /* 2^15 */);

    listener.onMS5611Values(P);

    loop_count++;
  }

  @Override public void run() {
    try {
      if (!setup())
        return;

      while (true)
        loop();
    } catch (ConnectionLostException e) {
      Log.d(TAG, "MS5611.run() failed", e);
    } catch (IllegalStateException e) {
      Log.d(TAG, "MS5611.run() failed", e);
    } catch (InterruptedException e) {
    } finally {
      listener.onMS5611Error();
    }
  }
}
