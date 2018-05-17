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
import java.io.InputStream;
import android.util.Log;

/**
 * A wrapper for an InputStream which allows reading with a timeout.
 */
class InputThread extends Thread {
  private static final String TAG = "XCSoar";

  static final int BUFFER_SIZE = 256;

  final String name;

  volatile InputListener listener;

  volatile InputStream is;

  InputThread(String _name, InputListener _listener, InputStream _is) {
    super("InputThread " + _name);

    name = _name;
    listener = _listener;
    is = _is;

    start();
  }

  void setListener(final InputListener _listener) {
    listener = _listener;
  }

  /**
   * Similar to close(), but is allowed to be called from this thread.
   */
  private void closeInternal() {
    InputStream is2 = is;
    if (is2 == null)
      return;

    is = null;
    listener = null;

    try {
      is2.close();
    } catch (IOException e) {
    }
  }

  void close() {
    closeInternal();

    try {
      join();
    } catch (InterruptedException e) {
    }
  }

  boolean isValid() {
    return is != null;
  }

  @Override public void run() {
    byte[] buffer = new byte[BUFFER_SIZE];

    InputStream is2 = is;
    while (is2 != null) {
      int n;
      try {
        n = is2.read(buffer, 0, buffer.length);
      } catch (IOException e) {
        if (is != null)
          Log.e(TAG, "Failed to read from " + name, e);

        closeInternal();
        break;
      }

      if (n < 0)
        break;

      is2 = is;
      if (is2 == null)
        // close() was called
        break;

      InputListener listener2 = listener;
      if (listener2 != null)
        listener2.dataReceived(buffer, n);
    }
  }
}
