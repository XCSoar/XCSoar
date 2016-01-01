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

/*
 * Nunchuck wires:
 *
 * Green	SDA
 * Yellow	SCLK
 * Red		3V3
 * White	GND
 */

package org.xcsoar;

import android.util.Log;
import ioio.lib.api.IOIO;
import ioio.lib.api.TwiMaster;
import ioio.lib.api.DigitalInput;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * A driver for the Nunchuck connected via IOIO.
 * It does not support more then one Nunchuck.
 *
 * We use a 100 KHz I2C clock because of radio interference reasons.
 *
 */
final class Nunchuck extends Thread {
  interface Listener {
    /**
     * @param all sorts of params
     */
    void onNunchuckValues(int joy_x, int joy_y, int accel_x, int accel_y, int accel_z, int switches);
    void onNunchuckError();
  };

  private static final String TAG = "XCSoar";

  private static final byte WII_NUN_ADDR = 0x52;
  private static final byte REG_DATA     = 0x00; /*  */
  private static final int  REG_CAL      = 0x20; /* Calibration data */

  private static byte[] data = new byte[6];
  private static int[] u_data = new int[6];
  private static byte[] dummy = new byte[0];

  private final int sleeptime;
  private final TwiMaster h_twi;
  private final DigitalInput h_pin19;
  private final DigitalInput h_pin20;
  private final DigitalInput h_pin21;
  private final DigitalInput h_pin22;
  private final DigitalInput h_pin23;
  private final DigitalInput h_pin24;

  private final Listener listener;

  // calibration data
  private int joy_x_0, joy_y_0, joy_x_sens, joy_y_sens;
  private int acc_x_0, acc_y_0, acc_z_0, acc_x_sens, acc_y_sens, acc_z_sens;

  public Nunchuck(IOIO ioio, int twiNum, int sample_rate, Listener _listener)
    throws ConnectionLostException {
    super("NUNCHUCK");

    h_twi = ioio.openTwiMaster(twiNum, TwiMaster.Rate.RATE_100KHz, false);
    h_pin19 = ioio.openDigitalInput(19, DigitalInput.Spec.Mode.PULL_UP);
    h_pin20 = ioio.openDigitalInput(20, DigitalInput.Spec.Mode.PULL_UP);
    h_pin21 = ioio.openDigitalInput(21, DigitalInput.Spec.Mode.PULL_UP);
    h_pin22 = ioio.openDigitalInput(22, DigitalInput.Spec.Mode.PULL_UP);
    h_pin23 = ioio.openDigitalInput(23, DigitalInput.Spec.Mode.PULL_UP);
    h_pin24 = ioio.openDigitalInput(24, DigitalInput.Spec.Mode.PULL_UP);

    listener = _listener;
    sleeptime = 1000 / sample_rate - 10;

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

  private int nunchuckDecodeByte (byte x) {
    return (int)((x ^ 0x17) + 0x17) & 0xff;
  }

  private boolean setup()
    throws ConnectionLostException, InterruptedException {

    byte[] cal = new byte [16];
    int[] u_cal = new int [16];

    // reset
    if (!h_twi.writeRead(WII_NUN_ADDR, false, new byte[]{0x40, 0x00}, 2, dummy, 0)) return false;
    sleep(50);

    // read calibration data
    if (!h_twi.writeRead(WII_NUN_ADDR, false, new byte[]{REG_CAL}, 1, dummy, 0)) return false;
    sleep(50);

    if (!h_twi.writeRead(WII_NUN_ADDR, false, dummy, 0, cal, cal.length)) return false;
    sleep(50);

    for (int i=0; i<cal.length; i++) {
      u_cal[i] = nunchuckDecodeByte(cal[i]);
    }

    int acc_x_1, acc_y_1, acc_z_1;
    // process calibration data, sensitivity data shifted left 16 bits
    joy_x_0 = u_cal[10]; joy_x_sens = 200*65536 / (u_cal[8]  - u_cal[9]);	// output joystick values from -99 ..  +99
    joy_y_0 = u_cal[13]; joy_y_sens = 200*65536 / (u_cal[11] - u_cal[12]);
    // u_cal[3] and [7] hold the lower 2 bits
    acc_x_0 = (u_cal[0] << 2) +  (u_cal[3] & 3);
    acc_y_0 = (u_cal[1] << 2) + ((u_cal[3] & 0xc) >> 2);
    acc_z_0 = (u_cal[2] << 2) + ((u_cal[3] & 0x30) >> 4);
    acc_x_1 = (u_cal[4] << 2) +  (u_cal[7] & 3);
    acc_y_1 = (u_cal[5] << 2) + ((u_cal[7] & 0xc) >> 2);
    acc_z_1 = (u_cal[6] << 2) + ((u_cal[7] & 0x30) >> 4);
     
    // u_cal[7] holds lower 2 bits
    acc_x_sens = 1000*65536 / (acc_x_1 - acc_x_0);	// mG per bit
    acc_y_sens = 1000*65536 / (acc_y_1 - acc_y_0);
    acc_z_sens = 1000*65536 / (acc_z_1 - acc_z_0);

    return true;
  }

  final static byte get_data[] = new byte[] { REG_DATA };
  private void readData() throws ConnectionLostException, InterruptedException {
    
    // 1. Send a request for data to the nunchuck
    h_twi.writeRead(WII_NUN_ADDR, false, get_data, get_data.length, dummy, 0);

    sleep(10);

    // 2. read data
    h_twi.writeRead(WII_NUN_ADDR, false, dummy, 0, data, data.length);
    for (int i=0; i < data.length; i++) u_data[i] = nunchuckDecodeByte(data[i]);
  }

  private void loop() throws ConnectionLostException, InterruptedException {

    int joy_x = 1000; int joy_y = 0; int acc_x = 0; int acc_y = 0; int acc_z = 0; int switches = 0;
    if (h_twi != null) {
      readData();
      joy_x = ((u_data[0] - joy_x_0) * joy_x_sens) >> 16;
      joy_y = ((u_data[1] - joy_y_0) * joy_y_sens) >> 16;
      acc_x = (((u_data[2] << 2) + ((u_data[5] & 0x0c) >> 2) - acc_x_0) * acc_x_sens) >> 16;
      acc_y = (((u_data[3] << 2) + ((u_data[5] & 0x30) >> 4) - acc_y_0) * acc_y_sens) >> 16;
      acc_z = (((u_data[4] << 2) + ((u_data[5] & 0xc0) >> 6) - acc_z_0) * acc_z_sens) >> 16;
      switches = u_data[5] & 3;
    }
    if (h_pin19 != null) if (h_pin19.read()) switches += 0x4;
    if (h_pin20 != null) if (h_pin20.read()) switches += 0x8;
    if (h_pin21 != null) if (h_pin21.read()) switches += 0x10;
    if (h_pin22 != null) if (h_pin22.read()) switches += 0x20;
    if (h_pin23 != null) if (h_pin23.read()) switches += 0x40;
    if (h_pin24 != null) if (h_pin24.read()) switches += 0x80;

    listener.onNunchuckValues(joy_x, joy_y, acc_x, acc_y, acc_z, switches);

    sleep(sleeptime);
  }

  @Override public void run() {
    try {
      if (!setup())
        return;

      while (true)
        loop();
    } catch (ConnectionLostException e) {
      Log.d(TAG, "Nunchuck.run() failed", e);
    } catch (IllegalStateException e) {
      Log.d(TAG, "Nunchuck.run() failed", e);
    } catch (InterruptedException e) {
    } finally {
      listener.onNunchuckError();
    }
  }
}
