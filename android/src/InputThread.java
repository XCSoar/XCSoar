/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
import java.io.InputStream;
import android.util.Log;

/**
 * A wrapper for an InputStream which allows reading with a timeout.
 */
class InputThread extends Thread {
  private static final String TAG = "XCSoar";

  static final int BUFFER_SIZE = 256;

  InputStream is;

  int timeout = 0;

  byte[] buffer = new byte[BUFFER_SIZE];
  int head, tail;

  InputThread(InputStream _is) {
    is = _is;

    start();
  }

  synchronized void close() {
    InputStream is2 = is;
    if (is2 == null)
      return;

    is = null;

    try {
      is2.close();
    } catch (IOException e) {
    }

    notifyAll();
  }

  synchronized void flush() {
    head = tail;
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
      while (true) {
        InputStream is2;

        if (tail >= BUFFER_SIZE) {
          synchronized(this) {
            while (is != null && head == 0) {
              try {
                wait();
              } catch (InterruptedException e) {
              }
            }

            is2 = is;
            shift();
          }
        } else
          is2 = is;

        if (is2 == null)
          // close() was called
          break;

        int n = is2.read(buffer, tail, BUFFER_SIZE - tail);
        if (n < 0)
          break;

        synchronized(this) {
          boolean wasEmpty = head >= tail;
          tail += n;

          if (wasEmpty)
            notifyAll();
        }
      }
    } catch (IOException e) {
      if (is != null)
        Log.e(TAG, "Failed to read from Bluetooth socket: " + e.getMessage());
    }
  }

  synchronized public int read() {
    if (head >= tail) {
      // buffer is empty
      if (timeout <= 0)
        return -1;

      // wait for the thread to fill the buffer
      try {
        wait(timeout);
      } catch (InterruptedException e) {
        return -1;
      }

      if (head >= tail)
        // still empty
        return -1;
    }

    // the result must be unsigned
    int ch = (int)buffer[head] & 0xff;
    ++head;

    if (tail >= BUFFER_SIZE)
      // notify the thread that it may continue reading
      notifyAll();

    return ch;
  }
}
