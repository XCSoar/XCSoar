// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;

class BatteryReceiver extends BroadcastReceiver {
  private static native void setBatteryPercent(int batteryPct, int plugged);

  @Override public void onReceive(Context context, Intent intent)
  {
    if (intent == null ||
        !intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
      return;
    }

    boolean isBatteryPresent =
        intent.getBooleanExtra(BatteryManager.EXTRA_PRESENT, false);
    int batteryPct = 0;
    int plugged = 1; // Assume plugged in if no battery is present

    if (isBatteryPresent) {
      int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
      int scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
      batteryPct = (level >= 0 && scale > 0) ? (level * 100 / scale) : 0;
      plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
    }

    // Pass the calculated battery percentage and plugged status to native code
    setBatteryPercent(batteryPct, plugged);
  }
}
