// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  public synchronized boolean increment() {
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
  public synchronized void decrement() {
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
