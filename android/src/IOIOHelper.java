/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
final class IOIOHelper extends Thread {
  private static final String TAG = "XCSoar";

  private enum Command {
    NONE,
    SHUTDOWN,
    OPEN,
    CLOSE,
  }

  /**
   * The command being sent to the connection thread.
   */
  private Command command = Command.NONE;

  private IOIO ioio_;

  /**
   * The IOIO connection that is currently being established.  It may
   * be non-null while running the OPEN command, and may be used by
   * the client thread to cancel the connection.
   */
  private IOIO connecting;

  private XCSUart[] xuarts_ = new XCSUart[4];

  public IOIOHelper() {
    super("IOIO");

    for (int i = 0; i < 4; i++)
      xuarts_[i] = new XCSUart(i);

    start();
  }

  private synchronized void wakeUp() {
    if (ioio_ != null)
      interrupt();
    else
      notifyAll();
  }

  private synchronized boolean waitCompletion() {
    while (command != Command.NONE) {
      try {
        wait();
      } catch (InterruptedException e) {
        return false;
      }
    }

    return true;
  }

  private synchronized boolean waitCompletion(int timeout_ms) {
    if (command == Command.NONE)
      return true;

    try {
      wait(timeout_ms);
    } catch (InterruptedException e) {
      return false;
    }

    return command == Command.NONE;
  }

  private synchronized boolean runCommand(Command _cmd)
    throws InterruptedException {
    if (command == _cmd)
      /* another thread is already running this command */
      return waitCompletion();

    if (!waitCompletion())
      return false;

    command = _cmd;
    wakeUp();
    return waitCompletion();
  }

  private synchronized boolean runCommand(Command _cmd, int timeout_ms) {
    if (command == _cmd)
      /* another thread is already running this command */
      return waitCompletion(timeout_ms);

    if (!waitCompletion(timeout_ms))
      return false;

    command = _cmd;
    wakeUp();
    return waitCompletion(timeout_ms);
  }

  /**
   * Called by the connection thread when a command has been executed.
   */
  private synchronized void commandFinished() {
    command = Command.NONE;
    notifyAll();
  }

  public void shutdown() {
    try {
      runCommand(Command.SHUTDOWN);
      join();
    } catch (InterruptedException e) {
    }
  }

  /**
   * Called by the connection thread to open a connection.
   */
  private void synchronousOpen() {
    if (ioio_ != null)
      return;

    IOIO ioio = connecting = IOIOFactory.create();

    try {
      ioio.waitForConnect();
      if (ioio.getState() == IOIO.State.CONNECTED) {
        Log.d(TAG, "IOIO connection established");

        Log.i(TAG, "IOIO hardware version " +
              ioio.getImplVersion(IOIO.VersionType.HARDWARE_VER));
        Log.i(TAG, "IOIO bootloader version " +
              ioio.getImplVersion(IOIO.VersionType.BOOTLOADER_VER));
        Log.i(TAG, "IOIO firmware version " +
              ioio.getImplVersion(IOIO.VersionType.APP_FIRMWARE_VER));
        Log.i(TAG, "IOIOLib version " +
              ioio.getImplVersion(IOIO.VersionType.IOIOLIB_VER));

        ioio.softReset();
        ioio_ = ioio;
      } else {
        Log.w(TAG, "IOIO connection failed");
        ioio.disconnect();
      }
    } catch (ConnectionLostException e) {
      Log.w(TAG, "IOIOJWaitConnect() Connection Lost", e);
      ioio.disconnect();
    } catch (IncompatibilityException e) {
      Log.e(TAG, "IOIOJWaitConnect() Incompatibility detected", e);
      ioio.disconnect();
    } finally {
      connecting = null;
    }
  }

  /**
   * Called by the connection thread to close the connection.
   */
  private synchronized void synchronousClose() {
    if (ioio_ == null)
      return;

    ioio_.disconnect();
    ioio_ = null;
  }

  /**
   * The connection thread.
   */
  public void run() {
    while (true) {
      synchronized(this) {
        if (ioio_ != null && ioio_.getState() != IOIO.State.CONNECTED)
          synchronousClose();
      }

      switch (command) {
      case NONE:
        /* idle: wait for a wakeUp() call */
        try {
          if (ioio_ != null) {
            /* there is a connection: wait for connection error or
               until Thread.interrupt() gets called */
            ioio_.waitForDisconnect();
            Log.w(TAG, "IOIO connection lost");
          } else {
            /* there is no connection: wait until Object.notify() gets
               called */
            synchronized(this) {
              wait();
            }
          }
        } catch (InterruptedException e) {
        }
        break;

      case SHUTDOWN:
        synchronousClose();
        commandFinished();
        return;

      case OPEN:
        synchronousOpen();
        commandFinished();
        break;

      case CLOSE:
        synchronousClose();
        commandFinished();
        break;
      }
    }
  }

  /**
   * Initializes the connection to the IOIO board.
   * Waits up to 3000ms to connect to the IOIO board.
   * @return: True if connection is successful. False if fails to 
   * connect after 3000ms.
   */
  public synchronized boolean open() {
    if (command == Command.OPEN)
      /* another thread is already opening the connecting */
      return waitCompletion();

    if (command == Command.NONE && ioio_ != null)
      return true;

    if (!runCommand(Command.OPEN, 3000)) {
      IOIO ioio = connecting;
      if (ioio != null)
        ioio.disconnect();
      return false;
    }

    return true;
  }

  /**
   * Disconnects the ioio board.
   * The board can be reopened by calling waitConnect()
   */
  public synchronized void close() {
    if (ioio_ == null)
      return;

    try {
      runCommand(Command.CLOSE);
    } catch (InterruptedException e) {
    }
  }

  /**
   * Is the IOIO connection currently in use?  If not, it may be
   * closed eventually.
   */
  private boolean isInUse() {
    for (XCSUart uart : xuarts_)
      if (!uart.isAvailable())
        return true;

    return false;
  }

  boolean autoOpen() {
    return ioio_ != null || open();
  }

  void autoClose() {
    if (ioio_ != null && !isInUse())
      close();
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
    private int ID;

    XCSUart(int ID_) {
      super("IOIO UART " + ID_);

      ID = ID_;

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
      return uart == null;
    }

    private boolean doOpen() {
      if (baudrate == 0)
        return false;

      IOIO ioio = ioio_;
      if (ioio == null)
        return false;

      try {
        uart = ioio.openUart(inPin, outPin, baudrate, Uart.Parity.NONE,
                             Uart.StopBits.ONE);
        super.set(uart.getInputStream(), uart.getOutputStream());
        return true;
      } catch (ConnectionLostException e) {
        Log.w(TAG, "IOIOJopenUart() Connection Lost.  Baud: " + baudrate, e);
        return false;
      } catch (Exception e) {
        Log.e(TAG, "IOIOJopenUart() Unexpected exception caught", e);
        return false;
      }
    }

    /**
     * opens the Uart
     * sets isAvailable to false to indicate that it is no longer 
     * available to open.
     * @return true on success
     */
    public boolean openUart(int _baud) {
      if (!isAvailable()) {
        Log.e(TAG, "IOIOJopenUart() is not available: " + ID);
        return false;
      }

      baudrate = _baud;
      return doOpen();
    }

    private void doClose() {
      super.close();

      try {
        uart.close();
      } catch (Exception e) {
        Log.e(TAG, "IOIOJclose() Unexpected exception caught", e);
      }
      uart = null;
    }

    /**
     * closes the Uart on the ioio
     * sets isAvailable to true to indicate it is available for reopening
     */
    public void close() {
      doClose();
      autoClose();

      /* delete the listener reference, to allow reusing this
         object */
      setListener(null);
    }

    public boolean setBaudRate(int baud) {
      doClose();
      baudrate = baud;
      boolean success = doOpen();

      /* check if the IOIO connection can be closed, just in case
         doOpen() has failed */
      autoClose();

      return success;
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
    if (!autoOpen())
      return null;

    XCSUart uart = xuarts_[ID];
    return uart.openUart(baud)
      ? uart
      : null;
  }
}
