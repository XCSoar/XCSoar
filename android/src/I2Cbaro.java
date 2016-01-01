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
 * TODO: split file in I2Cbaro.java, MS5611.java and BMP085.java
 *
 * A driver for the MS5611 and BMP085 pressure sensors, connected via IOIO.
 * @see http://http://www.meas-spec.com/downloads/ms5611-01ba01.pdf and
 * @see http://www.bosch-sensortec.com/content/language1/html/3477.htm
 *
 * We use a slow 100 KHz I2C clock because of radio interference reasons.
 *
 */
final class I2Cbaro extends Thread {
  interface Listener {
    /**
     * @param pressure the pressure [Pa]
     */
    void onI2CbaroValues(int sensor, int pressure);
    void onI2CbaroError();
  };

  private static final String TAG = "XCSoar";

  private int type = 0;
  private int sample_rate;
  private int sleep_time;

  /*
   * Then the simple things related to the BMP085
   */
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
  private int eocPin = 0;
  private DigitalInput h_eoc;

  private static final byte oversampling085 = 3;	// I see no reason to use anything else.

  private static final byte[] request085ChipID = new byte[] {
    BMP085_CHIP_ID_REG
  };

  private static final byte[] request085Parameters = new byte[] {
    BMP085_CALIBRATION_DATA_START
  };

  private static final byte[] read085Temperature = new byte[] {
    BMP085_CTRL_REG, BMP085_TEMP_MEASUREMENT
  };

  private static final byte[] request085Value = new byte[] {
    BMP085_CONVERSION_REGISTER_MSB
  };

  private byte[] read085Pressure = new byte[] {
    BMP085_CTRL_REG, BMP085_PRESSURE_MEASUREMENT
  };

  private byte[] response085Parameters = new byte [BMP085_CALIBRATION_DATA_LENGTH * 2];

  private byte[] response085 = new byte[3];

  /**
   * BMP085 pressure calibration coefficients.
   */
  private int ac1, ac2, ac3, ac4, b1, b2;

  /**
   * BMP085 temperature calibration coefficients.
   */
  private int ac5, ac6, mc, md;

  /*
   * And now the simple things related to the MS5611
   */
  static final byte CMD5611_RESET =              0x1E; /* ADC reset command */
  static final byte CMD5611_ADC_READ =           0x00; /* ADC read command */
  static final byte CMD5611_ADC_PRESS =          0x40; /* ADC D1 conversion */
  static final byte CMD5611_ADC_TEMP =           0x50; /* ADC D2 conversion */
  static final byte CMD5611_ADC_256 =            0x00; /* ADC OSR=256 */
  static final byte CMD5611_ADC_512 =            0x02; /* ADC OSR=512 */
  static final byte CMD5611_ADC_1024 =           0x04; /* ADC OSR=1024 */
  static final byte CMD5611_ADC_2048 =           0x06; /* ADC OSR=2048 */
  static final byte CMD5611_ADC_4096 =           0x08; /* ADC OSR=4096 */
  static final int  CMD5611_PROM_RD =            0xA0; /* Prom read command */

  static final byte oversampling5611 = CMD5611_ADC_4096;	// I see no reason to use anything else.

  private TwiMaster h_twi;
  private byte i2c_addr;
  private int flags;
  private final Listener listener;

  private byte[] request5611Caldata = new byte[1];

  private final byte[] request5611Reset = new byte[] {
    CMD5611_RESET
  };

  private final byte[] request5611Temp = new byte[] {
    CMD5611_ADC_TEMP + oversampling5611
  };

  private final byte[] request5611Press = new byte[] {
    CMD5611_ADC_PRESS + oversampling5611
  };

  private final byte[] request5611Value = new byte[] {
    CMD5611_ADC_READ
  };

  private byte[] response5611 = new byte[3];

  /**
   * MS5611 Pressure calibration coefficients, C1s and C5s are left shifted from C1 and C5.
   */
  private long C1s=0, C2s=0, C3=0, C4=0;

  /**
   * MS5611 Temperature calibration coefficients.
   */
  private int C5s=0; private long C6=0;


  private byte[] dummy = new byte [0];


  public I2Cbaro(IOIO ioio, int twiNum, int _i2c_addr, int _sample_rate, int _flags,
                Listener _listener)
    throws ConnectionLostException {
    super("I2Cbaro");

    h_twi = ioio.openTwiMaster(twiNum & 0xff, TwiMaster.Rate.RATE_100KHz, false);
    listener = _listener;
    if ((twiNum & 0xff00) == 0)
      i2c_addr =  (byte)_i2c_addr;
    else
      i2c_addr = (byte)(twiNum >> 8);
    eocPin = (byte)(twiNum >> 16);
    sample_rate = _sample_rate;
    flags = _flags;

    if (eocPin != 0)
      h_eoc = ioio.openDigitalInput(eocPin);

    start();
  }

  public void close() {
    if (h_eoc != null)
      h_eoc.close();

    if (h_twi != null)
      h_twi.close();

    interrupt();

    try {
      join();
    } catch (InterruptedException e) {
    }
  }

  private int readU16BE(byte[] data, int offset) {
    return ((data[offset] & 0xff) << 8) | (data[offset + 1] & 0xff);
  }

  private int readS16BE(byte[] data, int offset) {
    return (short)readU16BE(data, offset);
  }

  private int readU24BE(byte[] data, int offset) {
    return ((data[offset] & 0xff) << 16) | ((data[offset + 1] & 0xff) << 8)
      | (data[offset + 2] & 0xff);
  }

  private void reset5611()
    throws ConnectionLostException, InterruptedException {

    h_twi.writeRead(i2c_addr, false, request5611Reset, request5611Reset.length, dummy, 0);
    sleep(5);
  }

  private boolean setup085()
    throws ConnectionLostException, InterruptedException {
    /* is it a BMP085 sensor? */
    byte[] response085ChipID = new byte[1];
    h_twi.writeRead(i2c_addr, false,
                  request085ChipID, request085ChipID.length,
                  response085ChipID, response085ChipID.length);
    if (response085ChipID[0] != BMP085_CHIP_ID)
      return false;

    /* read the calibration coefficients */
    h_twi.writeRead(i2c_addr, false,
                  request085Parameters, request085Parameters.length,
                  response085Parameters, response085Parameters.length);

    ac1 = readS16BE(response085Parameters, 0);
    ac2 = readS16BE(response085Parameters, 2);
    ac3 = readS16BE(response085Parameters, 4);
    ac4 = readU16BE(response085Parameters, 6);
    ac5 = readU16BE(response085Parameters, 8);
    ac6 = readU16BE(response085Parameters, 10);
    b1 = readS16BE(response085Parameters, 12);
    b2 = readS16BE(response085Parameters, 14);
    mc = readS16BE(response085Parameters, 18);
    md = readS16BE(response085Parameters, 20);

    read085Pressure[1] += oversampling085 << 6;

    return true;
  }

  private boolean setup5611()
    throws ConnectionLostException, InterruptedException {

    int[] prom = new int [8];

    reset5611();

    /* Untill Checksum is implemented: too many zero results from EEPROM means no barometer present */
    int zeros = 0;

    /* Read Caldata from EEPROM */
    for (int i = 0; i<8; i++) {
      response5611[0]=0;response5611[1]=0;response5611[2]=0;
      request5611Caldata[0] = (byte)(CMD5611_PROM_RD + 2*i);
      h_twi.writeRead(i2c_addr, false, request5611Caldata, request5611Caldata.length, dummy, 0);
      h_twi.writeRead(i2c_addr, false, dummy, 0, response5611, 2);
      prom[i] = readU16BE(response5611, 0);
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

  private void read085Sensor(byte[] request, byte[] response, int responseLength)
    throws ConnectionLostException, InterruptedException {
    h_twi.writeRead(i2c_addr, false,
                  request, request.length, dummy, 0);

     if (h_eoc != null)
       h_eoc.waitForValue(true);
     else
       sleep(26);

    h_twi.writeRead(i2c_addr, false,
                  request085Value, request085Value.length,
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

    return (((ac1 * 4 + x3) << oversampling085) + 2) >> 2;
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
  private int get085Pressure(int up, int param_b5) {
    int b6 = param_b5 - 4000;
    int b3 = getB3(b6);
    int b4 = getB4(b6);

    int b7 = ((up - b3) * (50000 >> oversampling085));

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

  private int loop_count085 = 0;
  private int b5_085;
  private void loop085() throws ConnectionLostException, InterruptedException {
    if (loop_count085 % 10 == 0) {
      /* Start temp conversion but only every 10th loop. */
      read085Sensor(read085Temperature, response085, 2);
      b5_085 = getB5(readU16BE(response085, 0));
      /* double temperature_c = (double)((b5 + 8) >> 4) / 10.0; */
    }
    read085Sensor(read085Pressure, response085, 3);
    int up = readU24BE(response085, 0) >> (8 - oversampling085);
    int pressure_pa = get085Pressure(up, b5_085);

    listener.onI2CbaroValues(85, pressure_pa);
    loop_count085++;
    sleep(sleep_time);
  }

  private int read5611Sensor()
    throws ConnectionLostException, InterruptedException {
    h_twi.writeRead(i2c_addr, false, request5611Value, request5611Value.length, dummy, 0);
    h_twi.writeRead(i2c_addr, false, dummy, 0 , response5611, 3);
    return readU24BE(response5611, 0);
  }

  private int loop_count5611 = 0;
  private int TEMP;
  private long dT;
  private void loop5611() throws ConnectionLostException, InterruptedException {

    /* for some reason it does not work using shifts on long variables */

    int D1, P=0;

    /* Start temp conversion but only every 10th loop. */
    if (loop_count5611 % 10 == 0) {
	h_twi.writeRead(i2c_addr, false, request5611Temp, request5611Temp.length, dummy, 0);
        sleep(11);
        /* Read temp value and compute temperature */
        dT = read5611Sensor() - C5s;
        TEMP = (int)(2000L + ((dT * C6) >> 23));
    }

    /* Start pressure conversion. */
    h_twi.writeRead(i2c_addr, false, request5611Press, request5611Press.length, dummy, 0);
    sleep(11);

    /* Read pressure value */
    D1 = read5611Sensor();

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

    listener.onI2CbaroValues(5611, P);

    loop_count5611++;

    sleep(sleep_time);
  }

  @Override public void run() {
    try {
      if (i2c_addr == 0x77 && setup085())
        type = 85;
      else {
        if (setup5611())
          type = 5611;
        else {
         Log.e(TAG, "No supported barometer found.");
          return;
        }
      }

      if (type == 5611) {
        sleep_time = 1000 / sample_rate - 11; // - conversion time
        if (sleep_time < 1) sleep_time = 1;
        while (true)
          loop5611();
      }

      if (type == 85) {
        sleep_time = 1000 / sample_rate - 26; // - conversion time
        if (sleep_time < 1) sleep_time = 1;
        while (true)
          loop085();
      }

    } catch (ConnectionLostException e) {
      Log.d(TAG, "I2Cbaro.run() failed", e);
    } catch (IllegalStateException e) {
      Log.d(TAG, "I2Cbaro.run() failed", e);
    } catch (InterruptedException e) {
    } finally {
      listener.onI2CbaroError();
    }
  }
}
