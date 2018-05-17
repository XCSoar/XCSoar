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

import java.io.IOException;
import java.io.OutputStream;
import android.util.Log;

/**
 * A wrapper for an OutputStream which allows writing with a timeout.
 */
class OutputThread extends Thread {
  private static final String TAG = "XCSoar";

  static final int BUFFER_SIZE = 256;

  final String name;

  OutputStream os;

  int timeout = 0;

  byte[] buffer = new byte[BUFFER_SIZE];
  int head, tail;

  OutputThread(String _name, OutputStream _os) {
    super("InputThread " + _name);

    name = _name;
    os = _os;

    start();
  }

  synchronized void close() {
    OutputStream os2 = os;
    if (os2 == null)
      return;

    os = null;

    try {
      os2.close();
    } catch (IOException e) {
    }

    notifyAll();
  }

  synchronized boolean drain() {
    while (os != null && head < tail) {
      try {
        wait();
      } catch (InterruptedException e) {
        return false;
      }
    }

    return os != null;
  }

  void setTimeout(int _timeout) {
    timeout = _timeout;
  }

  private void shift() {
    System.arraycopy(buffer, head, buffer, 0, tail - head);
    tail -= head;
    head = 0;
  }

  @Override public void run() {
    try {
      byte[] copy = new byte[BUFFER_SIZE];

      while (true) {
        OutputStream os2;
        int size;

        synchronized(this) {
          while (os != null && head >= tail) {
            try {
              wait();
            } catch (InterruptedException e) {
            }
          }

          os2 = os;
          if (os2 == null)
            // close() was called
            break;

          size = tail - head;
          System.arraycopy(buffer, head, copy, 0, size);
        }

        os2.write(copy, 0, size);

        synchronized(this) {
          head += size;
          notifyAll();
        }
      }
    } catch (IOException e) {
      if (os != null)
        Log.e(TAG, "Failed to write to " + name, e);

      close();
    } finally {
      synchronized(this) {
        notifyAll();
      }
    }
  }

  public synchronized int write(byte[] data, int length) {
    if (os == null)
      return -1;

    if (tail >= BUFFER_SIZE) {
      if (head == 0) {
        // buffer is full

        if (timeout <= 0)
          return -1;

        try {
          wait(timeout);
        } catch (InterruptedException e) {
          return -1;
        }

        if (os == null || head == 0)
          // still full, timeout
          return -1;
      }

      shift();
    }

    final boolean was_empty = head == tail;
    int nbytes = BUFFER_SIZE - tail;
    if (nbytes > length)
      nbytes = length;

    System.arraycopy(data, 0, buffer, tail, nbytes);

    tail += nbytes;

    if (was_empty)
      // notify the thread that it may continue writing
      notifyAll();

    return nbytes;
  }
}
