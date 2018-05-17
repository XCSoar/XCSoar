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

import android.os.Build;
import android.view.MotionEvent;

/**
 * Forward MotionEvent object to EventBridge.
 */
abstract class DifferentTouchInput {
  public static DifferentTouchInput getInstance() {
    if (Build.VERSION.SDK_INT < 8)
      /* up to Android 2.1 Eclair */
      return SingleTouchInput.Holder.sInstance;
    else
      /* Android 2.2 Froyo or newer */
      return MultiTouchInput.Holder.sInstance;
  }

  public abstract void process(final MotionEvent event);

  /**
   * Implementation for Android versions that are not capable of
   * MultiTouch.
   */
  private static class SingleTouchInput extends DifferentTouchInput {
    private static class Holder {
      private static final SingleTouchInput sInstance = new SingleTouchInput();
    }

    public void process(final MotionEvent event) {
      switch (event.getAction()) {
      case MotionEvent.ACTION_DOWN:
        EventBridge.onMouseDown((int)event.getX(), (int)event.getY());
        break;

      case MotionEvent.ACTION_UP:
        EventBridge.onMouseUp((int)event.getX(), (int)event.getY());
        break;

      case MotionEvent.ACTION_MOVE:
        EventBridge.onMouseMove((int)event.getX(), (int)event.getY());
        break;
      }
    }
  }

  /**
   * Implementation for Android versions with MultiTouch support.
   */
  private static class MultiTouchInput extends DifferentTouchInput {
    private static class Holder {
      private static final MultiTouchInput sInstance = new MultiTouchInput();
    }

    public void process(final MotionEvent event) {
      switch (event.getActionMasked()) {
      case MotionEvent.ACTION_DOWN:
        EventBridge.onMouseDown((int)event.getX(), (int)event.getY());
        break;

      case MotionEvent.ACTION_UP:
        EventBridge.onMouseUp((int)event.getX(), (int)event.getY());
        break;

      case MotionEvent.ACTION_MOVE:
        EventBridge.onMouseMove((int)event.getX(), (int)event.getY());
        break;

      case MotionEvent.ACTION_POINTER_DOWN:
        EventBridge.onPointerDown();
        break;

      case MotionEvent.ACTION_POINTER_UP:
        EventBridge.onPointerUp();
        break;
      }
    }
  }
}
