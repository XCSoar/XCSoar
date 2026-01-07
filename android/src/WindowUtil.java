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
    
    /* Check if we're in landscape orientation */
    android.content.res.Configuration config = decorView.getContext().getResources().getConfiguration();
    boolean isLandscape = config.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE;
    
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      /* Android 11+ (API 30+): Use WindowInsetsController */
      WindowInsetsController controller = decorView.getWindowInsetsController();
      if (controller != null) {
        if (isLandscape) {
          /* Hide status bar in landscape to maximize screen space */
          controller.hide(WindowInsets.Type.statusBars());
        } else {
          /* Show status bar in portrait to display clock and battery */
          controller.show(WindowInsets.Type.statusBars());
        }
        /* Always show navigation bar */
        controller.show(WindowInsets.Type.navigationBars());
      }
    } else {
      /* Android 10 and below: Use setSystemUiVisibility */
      if (isLandscape) {
        /* Hide status bar in landscape */
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN);
      } else {
        /* Show status bar in portrait */
        decorView.setSystemUiVisibility(0);
      }
    }
  }

  static void enterFullScreenMode(Window window) {
    window.addFlags(FULL_SCREEN_WINDOW_FLAGS);
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

    /* Opt out of Android 15's edge-to-edge enforcement. This tells Android 15
       that we handle edge-to-edge ourselves, preventing letterboxing in
       horizontal mode. */
    if (Build.VERSION.SDK_INT >= 35) {
      try {
        java.lang.reflect.Method method = Window.class.getMethod("setOptOutEdgeToEdgeEnforcement", boolean.class);
        method.invoke(window, true);
      } catch (Exception e) {
        /* Method not available or reflection failed - ignore */
      }
    }
  }

  static void leaveFullScreenMode(Window window, int preserveFlags) {
    disableImmersiveMode(window);
    window.clearFlags(FULL_SCREEN_WINDOW_FLAGS & ~preserveFlags);

    /* Set display cutout mode to SHORT_EDGES in non-fullscreen mode to respect the notch.
       This allows content to extend into the cutout area while still accounting for it
       in safe area calculations. */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      WindowManager.LayoutParams lp = window.getAttributes();
      lp.layoutInDisplayCutoutMode =
        WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
      window.setAttributes(lp);
    }
  }
}
