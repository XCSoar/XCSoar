// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.app.Activity;
import android.view.KeyEvent;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;

/**
 * On Android 16 (API 36) and later, apps that target API 36 no longer
 * receive {@link KeyEvent#KEYCODE_BACK}.  Register a predictive-back
 * callback that injects the same Escape mapping EventBridge already
 * applies to the hardware Back key.
 *
 * Isolated in this class so older Android versions never load
 * {@link OnBackInvokedCallback}.
 */
class PredictiveBack {
  static Object register(Activity activity) {
    OnBackInvokedCallback callback = () -> {
      EventBridge.onKeyDown(KeyEvent.KEYCODE_BACK);
      EventBridge.onKeyUp(KeyEvent.KEYCODE_BACK);
    };

    activity.getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
      OnBackInvokedDispatcher.PRIORITY_DEFAULT, callback);
    return callback;
  }

  static void unregister(Activity activity, Object token) {
    activity.getOnBackInvokedDispatcher()
      .unregisterOnBackInvokedCallback((OnBackInvokedCallback) token);
  }
}
