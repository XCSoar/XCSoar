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

/**
 * The Java interface of the C++ AndroidPort class.
 */
interface AndroidPort {
  int STATE_READY = 0;
  int STATE_FAILED = 1;
  int STATE_LIMBO = 2;

  void setListener(PortListener listener);

  void setInputListener(InputListener listener);

  void close();

  int getState();

  /**
   * @see Port::Drain()
   */
  boolean drain();

  int getBaudRate();
  boolean setBaudRate(int baud);

  /**
   * Write data to the port.  Execution blocks until at least one
   * bytes is written or an error occurs or until the timeout expires.
   *
   * @param data the data to be written
   * @param length the number of bytes to be written
   * @return the number of bytes that were written or -1 on error/timeout
   */
  int write(byte[] data, int length);
}
