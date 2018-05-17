/* Copyright_License {

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
 * This class manages safe destruction of objects that call native
 * methods.  It keeps track of how many threads are currently running
 * inside native code, and allows waiting for this counter to become
 * zero.  As soon as that happens, the native code may free its
 * memory.
 */
class SafeDestruct {
  /**
   * The reference counter.
   */
  private int count = 0;

  /**
   * A flag that specifies whether shutdown was requested by
   * beginShutdown().  When set, the reference counter will never be
   * incremented.
   */
  private boolean shutdown = false;

  /**
   * Safely increment the reference counter.  Returns false when the
   * object is shutting down (no change to the reference counter in
   * this case).
   */
  public synchronized boolean Increment() {
    if (shutdown)
      return false;

    ++count;
    return true;
  }

  /**
   * Safely increment the reference counter.  Returns false when the
   * object is shutting down (no change to the reference counter in
   * this case).
   */
  public synchronized void Decrement() {
    --count;

    if (count == 0 && shutdown)
      /* notify the thread that is waiting in finishShutdown() */
      notifyAll();
  }

  /**
   * Initiate the shutdown of the managed object.  This method is
   * asynchronous, it returns immediately.
   */
  public synchronized void beginShutdown() {
    if (shutdown)
      throw new IllegalStateException("Already shutting down");

    shutdown = true;
  }

  /**
   * Synchronously wait for the shutdown to complete, i.e. waits for
   * the reference counter to drop to zero.
   */
  public synchronized void finishShutdown() {
    if (!shutdown)
      throw new IllegalStateException("Not shutting down");

    while (count > 0) {
      try {
        wait();
      } catch (InterruptedException e) {
      }
    }
  }
}
