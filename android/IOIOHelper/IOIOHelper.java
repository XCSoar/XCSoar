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

import java.util.Timer;
import java.util.TimerTask;
import android.util.Log;
import ioio.lib.api.IOIO;
import ioio.lib.api.IOIOFactory;
import ioio.lib.api.Uart;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.api.exception.IncompatibilityException;

/**
 * A utility class which wraps the Java API into an easier API for the
 * C++ code.
 */
final class IOIOHelper {

  private IOIO ioio_;
  private XCSUart[] xuarts_;

  /**
   * Initializes the connection to the IOIO board.
   * Waits up to 3000ms to connect to the IOIO board.
   * @return: True if connection is successful. False if fails to 
   * connect after 3000ms.
   */
  public boolean open() {
    ioio_ = IOIOFactory.create();
    xuarts_ = new XCSUart [4];
    for (int i = 0; i < 4; i++)
      xuarts_[i] = new XCSUart(i);
    return waitConnect();
  }
  
  /**
   * Disconnects the ioio board.
   * The board can be reopened by calling waitConnect()
   */
  public void close() {
    try {
      ioio_.disconnect();
    } catch (Exception e) {
      Log.e("IOIOHelper", "IOIOJclose()/disconnect Unexpected exception caught", e);
    } finally {
      ioio_ = null;
    }
  }

  /**
   * Waits up to 3000ms for connection to IOIO.
   * Does soft reset of IOIO Board and all ports
   * @return: true if connection to board is successful
   */
  private boolean waitConnect() {
    boolean result = false;
    Timer t = new Timer();
    try {
      t.schedule(new TimerTask() {
          @Override
          public void run() {
            Log.w("IOIOHelper", "IOIOJWaitConnect() TimerDisconnecting...");
            ioio_.disconnect();
          }
        }, 3000);
      ioio_.waitForConnect();
      ioio_.softReset();
      result = true;
    } catch (ConnectionLostException e) {
      Log.w("IOIOHelper", "IOIOJWaitConnect() Connection Lost", e);
    } catch (IncompatibilityException e) {
      Log.e("IOIOHelper", "IOIOJWaitConnect() Incompatibility detected", e);
    } catch (Exception e) {
      Log.e("IOIOHelper", "IOIOJWaitConnect() Unexpected exception caught", e);
    } finally {
      t.cancel();
      return result;
    }
  }

  /**
   * Supports array of 4 XCSUart with ID=0, 1, 2, 3
   *
   * ID = 0, pins TX=3, RX=4
   * ID = 1, pins TX=5, RX=6
   * ID = 2, pins TX=10 RX=11
   * ID = 3, pins TX=12 RX=13
   *
   */
  class XCSUart extends AbstractAndroidPort {
    private Uart uart;
    private final int inPin;
    private final int outPin;
    private int baudrate = 0;
    private boolean isAvailable;
    private int ID;

    XCSUart(int ID_) {
      super("IOIO UART " + ID_);

      ID = ID_;
      isAvailable = true;

      switch (ID) {
      case 0:
      case 1:
        inPin = (ID * 2) + 4;
        outPin = inPin - 1;
        break;
      case 2:
      case 3:
        inPin = (ID * 2) + 7;
        outPin = inPin - 1;
        break;
      default:
        throw new IllegalArgumentException();
      }
    }

    /**
     * returns true if the Uart is not in use and can be opened
     * returns false if Uart is already open
     */
    public boolean isAvailable() {
      return isAvailable;
    }

    /**
     * opens the Uart
     * sets isAvailable to false to indicate that it is no longer 
     * available to open.
     * @return true on success
     */
    public boolean openUart(int _baud) {
      if (!isAvailable) {
        Log.e("IOIOHelper", "IOIOJopenUart() is not available: " + ID);
        return false;
      }

      baudrate = _baud;
      try {
        uart = ioio_.openUart(inPin, outPin, baudrate, Uart.Parity.NONE,
                              Uart.StopBits.ONE);
      } catch (ConnectionLostException e) {
        Log.w("IOIOHelper", "IOIOJopenUart() Connection Lost.  Baud: " + _baud, e);
        return false;
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJopenUart() Unexpected exception caught", e);
        return false;
      }

      super.set(uart.getInputStream(), uart.getOutputStream());
      isAvailable = false;
      return true;
    }

    /**
     * closes the Uart on the ioio
     * sets isAvailable to true to indicate it is available for reopening
     */
    public void close() {
      super.close();

      try {
        uart.close();
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJclose() Unexpected exception caught", e);
      }
      uart = null;
      isAvailable = true;
    }

    public int setBaudRate(int baud) {
      close();
      openUart(baud);
      return baud;
    }

    public int getBaudRate() {
      return baudrate;
    }
  }

  /**
   * @ID: ID of UArt to open (0, 1, 2, 3)
   * @return: ID of opened UArt or -1 if fail
   */
  public AndroidPort openUart(int ID, int baud) {
    XCSUart uart = xuarts_[ID];
    return uart.openUart(baud)
      ? uart
      : null;
  }
}
