/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

import android.view.Window;
import android.view.WindowManager;
import android.view.View;

/**
 * A library of utility functions for class #Window.
 */
class WindowUtil {
  static final int FULL_SCREEN_WINDOW_FLAGS =
    WindowManager.LayoutParams.FLAG_FULLSCREEN|

    /* Workaround for layout problems in Android KitKat with immersive full
       screen mode: Sometimes the content view was not initialized with the
       correct size, which caused graphics artifacts. */
    WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN|
    WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS|
    WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR|
    WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION;

  /**
   * Set / Reset the System UI visibility flags for Immersive Full
   * Screen Mode.
   */
  static void enableImmersiveMode(Window window) {
    View decorView = window.getDecorView();
    decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|
                                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
                                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
                                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
                                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
                                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
  }

  static void disableImmersiveMode(Window window) {
    View decorView = window.getDecorView();
    decorView.setSystemUiVisibility(0);
  }

  static void enterFullScreenMode(Window window) {
    window.addFlags(FULL_SCREEN_WINDOW_FLAGS);
    enableImmersiveMode(window);
  }

  static void leaveFullScreenMode(Window window, int preserveFlags) {
    disableImmersiveMode(window);
    window.clearFlags(FULL_SCREEN_WINDOW_FLAGS & ~preserveFlags);
  }
}
