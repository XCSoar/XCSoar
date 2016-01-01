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
