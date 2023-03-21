// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * Listener for IOIO connection events.
 */
interface IOIOConnectionListener {
  /**
   * Invoked when the connection to a IOIO board was successfully
   * established.  Also invoked when this listener gets added to the
   * #IOIOHelper and there is already a connection.
   *
   * @param ioio the IOIO connection that was established
   * @throws ConnectionLostException when the IOIO connection failes
   * during this method
   * @throws InterruptedException when the current thread gets
   * interrupted; the caller may decide to invoke this method again
   */
  void onIOIOConnect(IOIO ioio)
    throws ConnectionLostException, InterruptedException;

  /**
   * Invoked when the connection to the IOIO has been torn down.  Also
   * invoked when this listener gets removed from the #IOIOHelper and
   * there is already a connection.
   *
   * @param ioio the IOIO connection that was disconnected; this is
   * the same object that was passed to onIOIOConnect()
   */
  void onIOIODisconnect(IOIO ioio);
}
