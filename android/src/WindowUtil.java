// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.os.Build;
import android.view.Window;
import android.view.WindowManager;
import android.view.View;
import android.view.WindowInsetsController;
import android.view.WindowInsets;

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
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      // Android 11+ (API 30): Use modern WindowInsetsController API
      WindowInsetsController controller = window.getInsetsController();
      if (controller != null) {
        controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
        controller.setSystemBarsBehavior(
          WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
      }
    } else {
      // Android 10 and below: Use deprecated setSystemUiVisibility
      View decorView = window.getDecorView();
      decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|
                                      View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
                                      View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
                                      View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
                                      View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
                                      View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }
  }

  static void disableImmersiveMode(Window window) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      // Android 11+ (API 30): Use modern WindowInsetsController API
      WindowInsetsController controller = window.getInsetsController();
      if (controller != null) {
        controller.show(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
        controller.setSystemBarsBehavior(
          WindowInsetsController.BEHAVIOR_DEFAULT);
      }
    } else {
      // Android 10 and below: Use deprecated setSystemUiVisibility
      View decorView = window.getDecorView();
      decorView.setSystemUiVisibility(0);
    }
  }

  static void enterFullScreenMode(Window window) {
    /* On Android 11+ (API 30), tell the window to draw behind system bars */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      window.setDecorFitsSystemWindows(false);
    }
    
    /* On Android 15+, the old FLAG_FULLSCREEN and FLAG_LAYOUT_* flags conflict
       with the new inset system. Clear them and set only essential flags. */
    if (Build.VERSION.SDK_INT < 35) {
      window.addFlags(FULL_SCREEN_WINDOW_FLAGS);
    } else {
      window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN |
                       WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN |
                       WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS |
                       WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR |
                       WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
      window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON |
                      WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
    }
    
    enableImmersiveMode(window);

    /* Enable display cutout mode (notch support) for Android P (API 28) and above.
       LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES allows content to extend into
       the cutout area on the short edges of the screen (top/bottom in portrait,
       left/right in landscape). This gives true edge-to-edge fullscreen. */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      WindowManager.LayoutParams lp = window.getAttributes();
      lp.layoutInDisplayCutoutMode =
        WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
      window.setAttributes(lp);
    }
  }

  static void leaveFullScreenMode(Window window, int preserveFlags) {
    disableImmersiveMode(window);
    
    /* On Android 11+ (API 30), restore system window insets */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      window.setDecorFitsSystemWindows(true);
    }
    
    window.clearFlags(FULL_SCREEN_WINDOW_FLAGS & ~preserveFlags);

    /* Reset display cutout mode to default when leaving fullscreen */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      WindowManager.LayoutParams lp = window.getAttributes();
      lp.layoutInDisplayCutoutMode =
        WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT;
      window.setAttributes(lp);
    }
  }
}
