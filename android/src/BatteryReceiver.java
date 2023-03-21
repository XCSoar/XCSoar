// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.os.BatteryManager;

class BatteryReceiver extends BroadcastReceiver {
  private static native void setBatteryPercent(int level, int plugged);

  @Override public void onReceive(Context context, Intent intent) {
    int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
    int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0);
    setBatteryPercent(level, plugged);
  }
}
