/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

import android.os.Handler;

/**
 * A periodic timer used by the native code.
 */
class Timer implements Runnable {
  static Handler handler;

  /**
   * Global initialization of the class.  Must be called from the main
   * event thread, because the Handler object must be bound to that
   * thread.
   */
  public static void Initialize() {
    handler = new Handler();
  }

  long ptr;
  int period;

  Timer(long _ptr, int _period) {
    ptr = _ptr;
    period = _period;
    install();
  }

  private void install() {
    handler.postDelayed(this, period);
  }

  private void uninstall() {
    handler.removeCallbacks(this);
  }

  private native void run(long ptr);

  @Override public void run() {
    run(ptr);
  }
}
