// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
