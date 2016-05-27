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

import java.util.Collection;
import java.util.LinkedList;
import java.util.Iterator;
import java.util.NoSuchElementException;
import android.util.Log;
import android.content.ContextWrapper;
import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.util.IOIOConnectionRegistry;
import ioio.lib.util.android.ContextWrapperDependent;
import ioio.lib.spi.IOIOConnectionBootstrap;
import ioio.lib.spi.IOIOConnectionFactory;

/**
 * A utility class which wraps the Java API into an easier API for the
 * C++ code.
 */
final class IOIOHelper implements IOIOConnectionHolder,
                                  IOIOAgent.Listener {
  private static final String TAG = "XCSoar";

  private IOIOMultiAgent agent;

  private IOIO ioio_;

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

  static {
    final String[] bootstraps = new String[]{
      "ioio.lib.impl.SocketIOIOConnectionBootstrap",
      "ioio.lib.android.accessory.AccessoryConnectionBootstrap",
      "ioio.lib.android.bluetooth.BluetoothIOIOConnectionBootstrap",
      "ioio.lib.android.device.DeviceConnectionBootstrap",
    };

    IOIOConnectionRegistry.addBootstraps(bootstraps);
  }

  static void onCreateContext(ContextWrapper context) {
    for (IOIOConnectionBootstrap bootstrap : IOIOConnectionRegistry.getBootstraps())
      if (bootstrap instanceof ContextWrapperDependent)
        ((ContextWrapperDependent) bootstrap).onCreate(context);
  }

  static void onDestroyContext() {
    for (IOIOConnectionBootstrap bootstrap : IOIOConnectionRegistry.getBootstraps())
      if (bootstrap instanceof ContextWrapperDependent)
        ((ContextWrapperDependent) bootstrap).onDestroy();
  }

  public IOIOHelper() {
    agent = new IOIOMultiAgent(IOIOConnectionRegistry.getConnectionFactories(),
                               this);
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
    if (agent != null)
      agent.shutdown();
  }

  private static void openListener(IOIOConnectionListener listener, IOIO ioio)
    throws ConnectionLostException {
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

  private synchronized void closeAllListeners(IOIO ioio) {
    for (Iterator<IOIOConnectionListener> i = openListeners.iterator(); i.hasNext();) {
      IOIOConnectionListener listener = i.next();
      listener.onIOIODisconnect(ioio);

      i.remove();
      closedListeners.add(listener);
    }
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

  @Override public synchronized void addListener(IOIOConnectionListener l) {
    closedListeners.add(l);

    agent.enable();

    for (IOIOConnectionBootstrap bootstrap : IOIOConnectionRegistry.getBootstraps()) {
      try {
        if (bootstrap instanceof ContextWrapperDependent)
          ((ContextWrapperDependent) bootstrap).open();
      } catch (Exception e) {
        Log.e(TAG, "ContextWrapperDependent.open() failed", e);
      }
    }
  }

  @Override public synchronized void removeListener(IOIOConnectionListener l) {
    if (openListeners.remove(l))
      /* this listener thinks the IOIO connection is established;
         invoke the onIOIODisconnect() method */
      l.onIOIODisconnect(ioio_);
    else
      /* no method call necessary */
      closedListeners.remove(l);

    if (!isInUse())
      agent.disable();
  }

  @Override
  public synchronized void cycleListener(IOIOConnectionListener l) {
    if (!isOpen())
      return;

    if (openListeners.remove(l)) {
      l.onIOIODisconnect(ioio_);
      closedListeners.add(l);
    } else if (!closedListeners.contains(l))
      return;

    /* ask the thread to reopen the listener */
    agent.wakeUp();
  }

  @Override public synchronized void onIOIOConnect(IOIO ioio)
    throws ConnectionLostException, InterruptedException {
    openAllListeners(ioio);
    ioio_ = ioio;
  }

  @Override public synchronized void onIOIODisconnect(IOIO ioio) {
    ioio_ = null;
    closeAllListeners(ioio);
  }

  @Override public synchronized boolean onIOIOIdle(IOIO ioio) {
    return handleNewListeners();
  }

  /**
   * @ID: ID of UArt to open (0, 1, 2, 3)
   * @return: ID of opened UArt or -1 if fail
   */
  public AndroidPort openUart(int ID, int baud) {
    return new GlueIOIOPort(this, ID, baud);
  }
}
