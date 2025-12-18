// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.os.Build;

/**
 * Utilities for registering broadcast receivers with proper API level compatibility.
 */
final class BroadcastUtil {
  /**
   * Register a broadcast receiver with proper API level compatibility.
   * On API 33+ (TIRAMISU), uses the 3-arg overload with RECEIVER_NOT_EXPORTED flag.
   * On API < 33, uses the 2-arg overload.
   */
  static void registerReceiver(Context context, BroadcastReceiver receiver,
                                IntentFilter filter) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      context.registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED);
    } else {
      context.registerReceiver(receiver, filter);
    }
  }
}
