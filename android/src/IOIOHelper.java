// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Collection;
import java.util.LinkedList;
import java.util.Iterator;
import java.util.NoSuchElementException;
import android.util.Log;
import android.content.ContextWrapper;
import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.util.android.ContextWrapperDependent;
import ioio.lib.spi.IOIOConnectionBootstrap;
import ioio.lib.spi.IOIOConnectionFactory;
import ioio.lib.spi.NoRuntimeSupportException;

/**
 * A utility class which wraps the Java API into an easier API for the
 * C++ code.
 */
final class IOIOHelper implements IOIOConnectionHolder,
                                  IOIOAgent.Listener {
  private static final String TAG = "XCSoar";

  private static final Collection<IOIOConnectionBootstrap> bootstraps =
    new LinkedList<IOIOConnectionBootstrap>();

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
    final String[] bootstrapClassNames = new String[]{
      "ioio.lib.impl.SocketIOIOConnectionBootstrap",
      "ioio.lib.android.accessory.AccessoryConnectionBootstrap",
      "ioio.lib.android.bluetooth.BluetoothIOIOConnectionBootstrap",
      "ioio.lib.android.device.DeviceConnectionBootstrap",
    };

    for (String className : bootstrapClassNames) {
        try {
            Class<? extends IOIOConnectionBootstrap> bootstrapClass = Class.forName(className).asSubclass(IOIOConnectionBootstrap.class);
            bootstraps.add(bootstrapClass.newInstance());
            Log.d(TAG, "Successfully added bootstrap class: " + className);
        } catch (ClassNotFoundException e) {
            Log.d(TAG, "Bootstrap class not found: " + className + ". Not adding.");
        } catch (NoRuntimeSupportException e) {
            Log.d(TAG, "No runtime support for: " + className + ". Not adding.");
        } catch (Throwable e) {
            Log.e(TAG, "Exception caught while attempting to initialize connection factory", e);
        }
    }
  }

  static void onCreateContext(ContextWrapper context) {
    for (IOIOConnectionBootstrap bootstrap : bootstraps)
      if (bootstrap instanceof ContextWrapperDependent)
        ((ContextWrapperDependent) bootstrap).onCreate(context);
  }

  static void onDestroyContext() {
    for (IOIOConnectionBootstrap bootstrap : bootstraps)
      if (bootstrap instanceof ContextWrapperDependent)
        ((ContextWrapperDependent) bootstrap).onDestroy();
  }

  public static Collection<IOIOConnectionFactory> getConnectionFactories() {
    Collection<IOIOConnectionFactory> result = new LinkedList<IOIOConnectionFactory>();
    for (IOIOConnectionBootstrap bootstrap : bootstraps) {
      try {
        bootstrap.getFactories(result);
      } catch (SecurityException e) {
        Log.e(TAG, "Failed to initialise IOIO bootstrap '" + bootstrap + "': " + e.getMessage());
      }
    }
    return result;
  }

  public IOIOHelper() {
    agent = new IOIOMultiAgent(getConnectionFactories(), this);
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

    for (IOIOConnectionBootstrap bootstrap : bootstraps) {
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
