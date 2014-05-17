/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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
import java.io.*;
import ioio.lib.api.IOIO;
import ioio.lib.api.AnalogInput;
import ioio.lib.api.DigitalInput;
import ioio.lib.api.TwiMaster;
import ioio.lib.api.SpiMaster;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * TODO: split file in Baro.java, MS5611.java and BMP085.java
 *
 * A driver for the Analog, MS5611 and BMP085 pressure sensors, connected via IOIO.
 * @see http://http://www.meas-spec.com/downloads/ms5611-01ba01.pdf and
 * @see http://www.bosch-sensortec.com/content/language1/html/3477.htm
 *
 * We use a slow 100 KHz I2C clock because of radio interference reasons.
 *
 */
abstract class Baro extends Thread {
  interface Listener {
    /**
     * @param pressure the pressure [Pa]
     */
    void onBaroValues(int sensor, int pressure);
    void onBaroError();
  };

  private static final String TAG = "XCSoar";

  protected int sleep_time;
  protected int flags;

  protected final Listener listener;

  public Baro(int _sample_rate, int _flags, Listener _listener) {
	super("Baro");
	
    sleep_time = 1000 / _sample_rate;
	flags = _flags;
	listener = _listener;
    listener.onBaroValues(0, 100);
	}
  public void close() {
    interrupt();

    try {
      join();
    } catch (InterruptedException e) {
    }
  }

  protected boolean setup() throws ConnectionLostException, InterruptedException { return true; }
  abstract protected void loop() throws ConnectionLostException, InterruptedException;
  
  @Override public void run() {
    try {
      listener.onBaroValues(0, -500);
      if (!setup()) listener.onBaroError();
      listener.onBaroValues(0, -600);
      while (true)
        loop();

    } catch (ConnectionLostException e) {
      Log.d(TAG, "Baro.run() failed", e);
    } catch (IllegalStateException e) {
      Log.d(TAG, "Baro.run() failed", e);
    } catch (InterruptedException e) {
    } finally {
      listener.onBaroError();
    }
  }
}

class BaroAnalog extends Baro
{
  private AnalogInput h_baro;

  public BaroAnalog(IOIO ioio, int _bus, int _sample_rate, int _flags, Listener _listener)
    throws ConnectionLostException {
    super(_sample_rate, _flags, _listener);

    h_baro = ioio.openAnalogInput(_bus + 31); // Analog input pins are 31 - 46

    start();
  }
  @Override public void close() {
    if (h_baro != null)
    	h_baro.close();

    super.close();
  }
  boolean led = true;
 @Override protected void loop() throws ConnectionLostException, InterruptedException {

  if (h_baro != null) {
	float u = h_baro.read();
    int p = (int)((u - 0.060606) * 7333); // configuration for MXP5010 sensor 
    listener.onBaroValues(0, p);
  }
  else listener.onBaroValues(0, -1);
	
  sleep(sleep_time);
  }
}

class BaroBMP085 extends Baro
{
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


  private byte[] dummy = new byte [0];


  private TwiMaster h_twi;
  private byte i2c_addr;
  private int eocPin = 0;
  private DigitalInput h_eoc;

  public BaroBMP085(IOIO ioio, int twiNum, int _i2c_addr, int _sample_rate, int _flags,
                Listener _listener)
    throws ConnectionLostException {
    super(_sample_rate, _flags, _listener);

    h_twi = ioio.openTwiMaster(twiNum & 0xff, TwiMaster.Rate.RATE_100KHz, false);
    if ((twiNum & 0xff00) == 0)
      i2c_addr =  (byte)_i2c_addr;
    else
      i2c_addr = (byte)(twiNum >> 8);
    eocPin = (byte)(twiNum >> 16);

    if (eocPin != 0)
      h_eoc = ioio.openDigitalInput(eocPin);

    if (sleep_time > 26) sleep_time -= 26; // - conversion time

    read085Pressure[1] += oversampling085 << 6;

    start();
  }

  @Override public void close() {
    if (h_twi != null)
        h_twi.close();

    super.close();
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

  @Override protected boolean setup()
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

    File file = new File("/sdcard/XCSoarData/debug.log");
    try {
	    FileOutputStream stream = new FileOutputStream(file);
	    try {
	        stream.write(("085 " + ac1 + "," + ac2 + "," + ac3 + "," + ac4 + "," + ac5 + "," + ac6 + "," + b1 + "," + b2).getBytes());
		} catch (IOException e) {
	    } finally {
	        stream.close();
	    }
	} catch (FileNotFoundException e) {
	} catch (IOException e) {
	}
   return true;
  }

  private void readSensor(byte[] request, byte[] response, int responseLength)
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
  private int getPressure(int up, int param_b5) {
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

  private int loop_count = 0;
  private int b5;
  @Override protected void loop() throws ConnectionLostException, InterruptedException {
    if (loop_count % 10 == 0) {
      /* Start temp conversion but only every 10th loop. */
      readSensor(read085Temperature, response085, 2);
      b5 = getB5(readU16BE(response085, 0));
      /* double temperature_c = (double)((b5 + 8) >> 4) / 10.0; */
    }
    readSensor(read085Pressure, response085, 3);
    int up = readU24BE(response085, 0) >> (8 - oversampling085);
    int pressure_pa = getPressure(up, b5);

    listener.onBaroValues(85, pressure_pa);
    loop_count++;
    sleep(sleep_time);
  }
}

class BaroMS5611 extends Baro
{
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


  private SpiMaster h_spi;
  private IOIOBus h_bus;

  public BaroMS5611(IOIO ioio, int bus, int addr, int _sample_rate, int _flags,
                Listener _listener)
    throws ConnectionLostException {
	super(_sample_rate, _flags, _listener);
	h_bus = IOIOBus.Create(ioio, bus, addr);

    start();
  }

  public void close() {
    if (h_bus != null)
    	h_bus.close();

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

  private void reset()
    throws ConnectionLostException, InterruptedException {

	h_bus.write(request5611Reset, request5611Reset.length);
    sleep(5);
  }

  @Override protected boolean setup()
    throws ConnectionLostException, InterruptedException {

    int[] prom = new int [8];

    reset();
    sleep(3);

    /* Untill Checksum is implemented: too many zero results from EEPROM means no barometer present */
    int zeros = 0;

    /* Read Caldata from EEPROM */
    for (int i = 0; i<8; i++) {
      response5611[0]=0;response5611[1]=0;response5611[2]=0;
      request5611Caldata[0] = (byte)(CMD5611_PROM_RD + 2*i);
      h_bus.writeRead(request5611Caldata, request5611Caldata.length, response5611, 2);
//      prom[i] = ((response5611[0] >= 0 ? response5611[0] : response5611[0] +256)*256)+(response5611[1] >= 0 ? response5611[1] : response5611[1] +256);
      prom[i] = readU16BE(response5611, 0);
      if (prom[i] == 0 || prom[i] == 65535) zeros++;
    }
/*    File file = new File("/sdcard/XCSoarData/debug.log");
    try {
	    FileOutputStream stream = new FileOutputStream(file);
	    try {
	        stream.write(("" + prom[0] + "," + prom[1] + "," + prom[2] + "," + prom[3] + "," + prom[4] + "," + prom[5] + "," + prom[6] + "," + prom[7]).getBytes());
		} catch (IOException e) {
	    } finally {
	        stream.close();
	    }
	} catch (FileNotFoundException e) {
	} catch (IOException e) {
	}*/

    // TODO: checksum
    if (zeros > 2)
        return false;

    C1s = ((long)prom[1]) * 32768L;
    C2s = ((long)prom[2]) * 65536L;
    C3  = prom[3];
    C4  = prom[4];
    C5s = prom[5] * 256;
    C6  = prom[6];

    if (sleep_time > 11) sleep_time -= 11; // - conversion time

    return true;
  }

  private int readSensor()
    throws ConnectionLostException, InterruptedException {
	  h_bus.writeRead(request5611Value, request5611Value.length, response5611, 3);
	  return readU24BE(response5611, 0);
  }

  private int loop_count = 0;
  private int TEMP;
  private long dT;
  @Override protected void loop() throws ConnectionLostException, InterruptedException {

    /* for some reason it does not work using shifts on long variables */

    int D1, P=0;

    /* Start temp conversion but only every 10th loop. */
    if (loop_count % 10 == 0) {
    	h_bus.write(request5611Temp, request5611Temp.length);
        sleep(11);
        /* Read temp value and compute temperature */
        dT = readSensor() - C5s;
        TEMP = (int)(2000L + ((dT * C6) >> 23));
    }

    /* Start pressure conversion. */
    h_bus.write(request5611Press, request5611Press.length);
    sleep(11);

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

    listener.onBaroValues(5611, P);

    loop_count++;

    sleep(sleep_time);
  }
}
