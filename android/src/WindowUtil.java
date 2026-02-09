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
  /**
   * Legacy fullscreen flags for Android 10 and below.
   *
   * FLAG_LAYOUT_NO_LIMITS is a KitKat-era workaround that tells the
   * system the window extends beyond the screen.  On modern Android
   * this conflicts with setDecorFitsSystemWindows(false) and can
   * suppress gesture navigation indicators on some OEMs (e.g. Nokia).
   *
   * On API 30+ we rely on setDecorFitsSystemWindows(false) +
   * WindowInsetsController instead and do not set these flags.
   */
  static final int LEGACY_FULL_SCREEN_FLAGS =
    WindowManager.LayoutParams.FLAG_FULLSCREEN|
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
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      /* Android 11+ (API 30+): Use WindowInsetsController */
      WindowInsetsController controller = decorView.getWindowInsetsController();
      if (controller != null) {
        controller.hide(WindowInsets.Type.systemBars());
        controller.setSystemBarsBehavior(
          WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
      }
    } else {
      /* Android 10 and below: Use deprecated setSystemUiVisibility */
      decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|
                                      View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
                                      View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
                                      View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
                                      View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
                                      View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }
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
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
      /* Pre-Android 11: use legacy flags for edge-to-edge layout */
      window.addFlags(LEGACY_FULL_SCREEN_FLAGS);
    }

    enableImmersiveMode(window);

    /* Enable display cutout mode (notch support) for Android P (API 28) and above.
       For SDK 35+, use LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS as the SHORT_EDGES mode
       is deprecated. For older versions, use SHORT_EDGES which allows content to
       extend into the cutout area on the short edges of the screen. */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      WindowManager.LayoutParams lp = window.getAttributes();
      if (Build.VERSION.SDK_INT >= 35) {
        lp.layoutInDisplayCutoutMode =
          WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
      } else {
        lp.layoutInDisplayCutoutMode =
          WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
      }
      window.setAttributes(lp);
    }
  }

  static void leaveFullScreenMode(Window window, int preserveFlags) {
    disableImmersiveMode(window);
    window.clearFlags(LEGACY_FULL_SCREEN_FLAGS & ~preserveFlags);

    /* Set display cutout mode in non-fullscreen mode to respect the notch.
       For SDK 35+, use LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS as the SHORT_EDGES mode
       is deprecated. This allows content to extend into the cutout area while still
       accounting for it in safe area calculations. */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      WindowManager.LayoutParams lp = window.getAttributes();
      if (Build.VERSION.SDK_INT >= 35) {
        lp.layoutInDisplayCutoutMode =
          WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
      } else {
        lp.layoutInDisplayCutoutMode =
          WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
      }
      window.setAttributes(lp);
    }
  }
}
