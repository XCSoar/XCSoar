/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

import ioio.lib.api.DigitalOutput;
import ioio.lib.api.IOIO;
import ioio.lib.api.IOIOFactory;
import ioio.lib.api.Uart;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.api.exception.IncompatibilityException;

import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;
import java.util.Set;
import java.io.InterruptedIOException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import android.util.Log;



/**
 * A utility class which wraps the Java API into an easier API for the
 * C++ code.
 */
final class IOIOHelper {

  private IOIO ioio_;
  private XCSUart[] xuarts_;

  public boolean open() {
    ioio_ = IOIOFactory.create();
    xuarts_ = new XCSUart [4];
    for (int i = 0; i < 4; i++)
      try {
        xuarts_[i] = new XCSUart(i);
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJIOIOHelper() fail " + i, e);
      }
    return waitConnect();
  }
  
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
   * non-blocking connection to IOIO board
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
  class XCSUart {
    private IOIO ioio_;
    private Uart uart;
    private InputStream input;
    private OutputThread output;
    private int inputTimeout = 50;
    private final int inPin;
    private final int outPin;
    private int baudrate = 0;
    private boolean isAvailable;
    private int ID;

    XCSUart(int ID_) throws Exception {
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
        throw new Exception();
      }
    }

    public boolean isAvailable() {
      return isAvailable;
    }

    public int openUart(IOIO ioio, int _baud) {
      if (!isAvailable) {
        Log.e("IOIOHelper", "IOIOJopenUart() is not available: " + ID);
        return -1;
      }

      ioio_ = ioio;
      baudrate = _baud;
      try {
        uart = ioio_.openUart(inPin, outPin, baudrate, Uart.Parity.NONE,
                              Uart.StopBits.ONE);
      } catch (ConnectionLostException e) {
        Log.w("IOIOHelper", "IOIOJopenUart() Connection Lost.  Baud: " + _baud, e);
        return -1;
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJopenUart() Unexpected exception caught", e);
        return -1;
      }
      input = uart.getInputStream();
      output = new OutputThread(uart.getOutputStream());
      output.setTimeout(5000);
      isAvailable = false;
      return ID;
    }

    /**
     * non-blocking connection to IOIO board
     */
    private boolean WaitConnect() {
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

    private void closeUart() {
      try {
        input.close();
        output.close();
        uart.close();
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJclose() Unexpected exception caught", e);
      }
      uart = null;
      isAvailable = true;
    }

    public int setBaudRate(int baud) {
      closeUart();
      openUart(ioio_, baud);
      return baud;
    }

    public int getBaudRate() {
      return baudrate;
    }

    public void setReadTimeout(int timeout) {
      inputTimeout = timeout;
    }

    synchronized public int read() {
      try {
        final int step = 10;
        int timeleft = inputTimeout;
        while (timeleft > 0 && input.available() <= 0) {
          wait(step);
          timeleft -= step;
        }
        if (input.available() <= 0)
          return -1;

        int r = input.read() & 0xFF;
        return r;
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJRead() Unexpected exception caught", e);
        return -1;
      }
    }

    public void write(byte ch) {
      if (uart == null)
        return;

      try {
        output.write(ch);
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJwrite() Unexpected exception caught", e);
      }
    }

    public void flush() {
      if (uart == null)
        return;

      try {
        final int toskip = input.available();
        if (toskip > 0)
          input.skip(toskip);
      } catch (Exception e) {
        Log.e("IOIOHelper", "IOIOJflush() Unexpected exception caught", e);
      }
    }
  }

  /**
   * @ID: ID of UArt to open (0, 1, 2, 3)
   * @return: ID of opened UArt or -1 if fail
   */
  public int openUart(int ID, int baud) {
    if (xuarts_[ID].isAvailable())
      return xuarts_[ID].openUart(ioio_, baud);
    return -1;
  }

  public void closeUart(int ID) {
    if (!xuarts_[ID].isAvailable())
      xuarts_[ID].closeUart();
  }

  public int setBaudRate(int ID, int baud) {
    return xuarts_[ID].setBaudRate(baud);
  }

  public int getBaudRate(int ID) {
    return xuarts_[ID].getBaudRate();
  }

  public void setReadTimeout(int ID, int timeout) {
    xuarts_[ID].setReadTimeout(timeout);
  }

  public int read(int ID) {
    return xuarts_[ID].read();
  }

  public void write(int ID, byte ch) {
    xuarts_[ID].write(ch);
  }

  public void flush(int ID) {
    xuarts_[ID].flush();
  }
}
