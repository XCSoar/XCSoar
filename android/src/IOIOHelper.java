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

import java.util.Collection;
import java.util.LinkedList;
import java.util.Iterator;
import android.util.Log;
import ioio.lib.api.IOIO;
import ioio.lib.api.IOIOFactory;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.api.exception.IncompatibilityException;

/**
 * A utility class which wraps the Java API into an easier API for the
 * C++ code.
 */
final class IOIOHelper extends Thread implements IOIOConnectionHolder {
  private static final String TAG = "XCSoar";

  private boolean shutdownFlag;

  private IOIO ioio_;

  /**
   * The IOIO connection that is currently being established.  It may
   * be used by the client thread to cancel the connection.
   */
  private IOIO connecting;

  /**
   * The list of listeners that believe there is no IOIO connection.
   */
  private Collection<IOIOConnectionListener> closedListeners =
    new LinkedList<IOIOConnectionListener>();

  /**
   * The list of listeners that believe a IOIO connection is
   * established.
   */
  private Collection<IOIOConnectionListener> openListeners =
    new LinkedList<IOIOConnectionListener>();

  public IOIOHelper() {
    super("IOIO");

    start();
  }

  private synchronized boolean isOpen() {
    return ioio_ != null;
  }

  /**
   * Is the IOIO connection currently in use?  If not, it may be
   * closed eventually.
   */
  private synchronized boolean isInUse() {
    return !openListeners.isEmpty() || !closedListeners.isEmpty();
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

  private static void openListener(IOIOConnectionListener listener, IOIO ioio)
    throws ConnectionLostException {
    /* clear this thread's "interrupted" flag, to avoid interrupting
       the listener */
    interrupted();

    try {
      try {
        listener.onIOIOConnect(ioio);
      } catch (InterruptedException e) {
        /* got an interrupt from another thread; try again */
        listener.onIOIOConnect(ioio);
      }
    } catch (InterruptedException e) {
      Log.e(TAG, "Failed to open IOIO device " + listener, e);
    } catch (RuntimeException e) {
      Log.e(TAG, "Failed to open IOIO device " + listener, e);
    }
  }

  private synchronized void openAllListeners(IOIO ioio)
    throws ConnectionLostException {
    for (Iterator<IOIOConnectionListener> i = closedListeners.iterator(); i.hasNext();) {
      IOIOConnectionListener listener = i.next();

      openListener(listener, ioio);

      i.remove();
      openListeners.add(listener);
    }
  }

  private synchronized void closeAllListeners() {
    for (Iterator<IOIOConnectionListener> i = openListeners.iterator(); i.hasNext();) {
      IOIOConnectionListener listener = i.next();
      listener.onIOIODisconnect();

      i.remove();
      closedListeners.add(listener);
    }
  }

  /**
   * Called by the connection thread to open a connection.
   */
  private void synchronousOpen() {
    Log.d(TAG, "open IOIO");

    IOIO ioio;

    synchronized(this) {
      if (shutdownFlag || interrupted())
        return;

      connecting = ioio = IOIOFactory.create();
    }

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

        synchronized(this) {
          openAllListeners(ioio);
          ioio_ = ioio;
        }
      } else {
        Log.w(TAG, "IOIO connection failed");
        ioio.disconnect();
      }
    } catch (ConnectionLostException e) {
      Log.w(TAG, "IOIO connection lost");
      ioio.disconnect();
    } catch (IncompatibilityException e) {
      Log.e(TAG, "IOIO incompatible", e);
      ioio.disconnect();
    } finally {
      synchronized(this) {
        connecting = null;
      }
    }
  }

  private synchronized IOIO steal() {
    IOIO ioio = this.ioio_;
    this.ioio_ = null;
    return ioio;
  }

  /**
   * Called by the connection thread to close the connection.
   */
  private void synchronousClose() {
    IOIO ioio = steal();
    if (ioio == null)
      return;

    closeAllListeners();
    ioio.disconnect();
  }

  private synchronized boolean handleNewListeners() {
    if (ioio_ == null || closedListeners.isEmpty())
      return false;

    /* another thread has registered new listeners; notify them
       that a IOIO connection exists */

    try {
      openAllListeners(ioio_);
    } catch (ConnectionLostException e) {
      Log.w(TAG, "IOIO connection lost");
    }

    return true;
  }

  private void idle() {
    if (interrupted())
      return;

    try {
      if (ioio_ != null) {
        /* there is a connection: wait for connection error or
           until Thread.interrupt() gets called */
        ioio_.waitForDisconnect();
        Log.w(TAG, "IOIO connection lost");
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
  public void run() {
    while (true) {
      if (ioio_ != null && (ioio_.getState() != IOIO.State.CONNECTED ||
                            !isInUse()))
        synchronousClose();
      else if (ioio_ == null && isInUse() && !shutdownFlag)
        synchronousOpen();

      if (shutdownFlag) {
        synchronousClose();
        return;
      }

      if (handleNewListeners())
        continue;

      idle();
    }
  }

  @Override public synchronized void addListener(IOIOConnectionListener l) {
    closedListeners.add(l);

    /* ask the thread to initiate a connection or to invoke
       onIOIOConnect() if the connection is already established */
    interrupt();
  }

  @Override public synchronized void removeListener(IOIOConnectionListener l) {
    if (openListeners.remove(l))
      /* this listener thinks the IOIO connection is established;
         invoke the onIOIODisconnect() method */
      l.onIOIODisconnect();
    else
      /* no method call necessary */
      closedListeners.remove(l);

    if (!isInUse()) {
      if (isOpen())
        /* ask the thread to auto-close the connection */
        interrupt();

      if (connecting != null)
        /* cancel the current connect attempt after the last listener
           got removed */
        connecting.disconnect();
    }
  }

  @Override
  public synchronized void cycleListener(IOIOConnectionListener l) {
    if (!isOpen())
      return;

    if (openListeners.remove(l)) {
      l.onIOIODisconnect();
      closedListeners.add(l);
    } else if (!closedListeners.contains(l))
      return;

    /* ask the thread to reopen the listener */
    interrupt();
  }

  /**
   * @ID: ID of UArt to open (0, 1, 2, 3)
   * @return: ID of opened UArt or -1 if fail
   */
  public AndroidPort openUart(int ID, int baud) {
    return new GlueIOIOPort(this, ID, baud);
  }
}
