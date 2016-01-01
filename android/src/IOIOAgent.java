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
import ioio.lib.api.IOIOFactory;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.api.exception.IncompatibilityException;
import ioio.lib.spi.IOIOConnectionFactory;

/**
 * This class attempts to establish a connection to a IOIO through a
 * specified IOIOConnectionFactory object.  Multiple instances of this
 * are used by #IOIOHelper, one for each registered
 * IOIOConnectionFactory.
 */
final class IOIOAgent extends Thread {
  private static final String TAG = "XCSoar";

  interface Listener extends IOIOConnectionListener {
    boolean onIOIOIdle(IOIO ioio)
      throws ConnectionLostException, InterruptedException;
  }

  private final IOIOConnectionFactory factory;

  private final Listener listener;

  private boolean openFlag, enabledFlag, shutdownFlag;

  /**
   * The IOIO connection that is currently being established.  It may
   * be used by the client thread to cancel the connection.
   */
  private IOIO connecting;

  public IOIOAgent(IOIOConnectionFactory _factory, Listener _listener) {
    super("IOIOAgent[" + _factory.getType() + "]");

    factory = _factory;
    listener = _listener;

    start();
  }

  public synchronized void enable() {
    enabledFlag = true;

    /* ask the thread to initiate a connection or to invoke
       onIOIOConnect() if the connection is already established */
    if (connecting == null)
      interrupt();
  }

  public synchronized void disable() {
    if (!enabledFlag)
      return;

    enabledFlag = false;

    if (openFlag)
      /* ask the thread to auto-close the connection */
      interrupt();

    if (connecting != null)
      /* cancel the current connect attempt */
      connecting.disconnect();
  }

  public void wakeUp() {
    interrupt();
  }

  public void shutdown() {
    synchronized(this) {
      shutdownFlag = true;

      IOIO ioio = connecting;
      if (ioio != null)
        ioio.disconnect();
    }

    interrupt();

    try {
      join();
    } catch (InterruptedException e) {
    }
  }

  /**
   * Called by the connection thread to open a connection.
   */
  private IOIO synchronousOpen() {
    Log.d(TAG, "open " + getName());

    IOIO ioio;

    synchronized(this) {
      if (shutdownFlag || interrupted())
        return null;

      connecting = ioio = IOIOFactory.create(factory.createConnection());
    }

    try {
      ioio.waitForConnect();
      if (ioio.getState() != IOIO.State.CONNECTED) {
        Log.w(TAG, "IOIO connection " + getName() + " failed");
        ioio.disconnect();
        return null;
      }

      Log.d(TAG, "IOIO connection " + getName() + " established");

      Log.i(TAG, "IOIO hardware version " +
            ioio.getImplVersion(IOIO.VersionType.HARDWARE_VER));
      Log.i(TAG, "IOIO bootloader version " +
            ioio.getImplVersion(IOIO.VersionType.BOOTLOADER_VER));
      Log.i(TAG, "IOIO firmware version " +
            ioio.getImplVersion(IOIO.VersionType.APP_FIRMWARE_VER));
      Log.i(TAG, "IOIOLib version " +
            ioio.getImplVersion(IOIO.VersionType.IOIOLIB_VER));

      ioio.softReset();

      synchronized(this) {
        listener.onIOIOConnect(ioio);
        openFlag = true;
        return ioio;
      }
    } catch (ConnectionLostException e) {
      Log.w(TAG, "IOIO connection " + getName() + " lost");
      ioio.disconnect();
      return null;
    } catch (IncompatibilityException e) {
      Log.e(TAG, "IOIO connection " + getName() + " incompatible");
      ioio.disconnect();
      return null;
    } catch (InterruptedException e) {
      ioio.disconnect();
      return null;
    } finally {
      synchronized(this) {
        connecting = null;
      }
    }
  }

  /**
   * Called by the connection thread to close the connection.
   */
  private void synchronousClose(IOIO ioio) {
    if (ioio == null)
      return;

    openFlag = false;
    listener.onIOIODisconnect(ioio);
    ioio.disconnect();
  }

  private void idle(IOIO ioio) {
    if (interrupted())
      return;

    try {
      if (listener.onIOIOIdle(ioio))
        return;
    } catch (ConnectionLostException e) {
      Log.w(TAG, "IOIO connection " + getName() + " lost");
      return;
    } catch (InterruptedException e) {
      return;
    }

    if (interrupted())
      return;

    try {
      if (ioio != null) {
        /* there is a connection: wait for connection error or
           until Thread.interrupt() gets called */
        ioio.waitForDisconnect();
        Log.w(TAG, "IOIO connection " + getName() + " lost");
      } else {
        /* there is no connection: wait until Thread.interrupt()
           gets called */
        synchronized(this) {
          /* we're not actually waiting for an Object.notify() call,
             this is just a dummy call to make this thread idle and
             interruptible */
          wait();
        }
      }
    } catch (InterruptedException e) {
    }
  }

  /**
   * The connection thread.
   */
  @Override public void run() {
    IOIO ioio = null;

    while (true) {
      if (ioio != null && (ioio.getState() != IOIO.State.CONNECTED ||
                           !enabledFlag)) {
        synchronousClose(ioio);
        ioio = null;
      } else if (ioio == null && enabledFlag && !shutdownFlag)
        ioio = synchronousOpen();

      if (shutdownFlag) {
        synchronousClose(ioio);
        return;
      }

      idle(ioio);
    }
  }
}
