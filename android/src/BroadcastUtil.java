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
   * On API 26+ (O), uses the 3-arg overload with flags.
   * On API < 26, uses the 2-arg overload.
   * Flags are set to RECEIVER_NOT_EXPORTED on API 33+ (TIRAMISU), otherwise 0.
   */
  static void registerReceiver(Context context, BroadcastReceiver receiver,
                                IntentFilter filter) {
    int flags = 0;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      flags = Context.RECEIVER_NOT_EXPORTED;
    }
    
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      context.registerReceiver(receiver, filter, flags);
    } else {
      context.registerReceiver(receiver, filter);
    }
  }
}

