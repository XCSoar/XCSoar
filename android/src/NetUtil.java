// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

/**
 * Utilities for dealing with networking.
 */
final class NetUtil {
  private static final String TAG = "XCSoar";

  private static final int STATE_UNKNOWN = 0;
  private static final int STATE_DISCONNECTED = 1;
  private static final int STATE_CONNECTED = 2;
  private static final int STATE_ROAMING = 3;

  private static ConnectivityManager cm;

  public static void initialise(Context context) {
    cm = (ConnectivityManager)
      context.getSystemService(Context.CONNECTIVITY_SERVICE);
  }

  public static int getNetState() {
    if (cm == null)
      return STATE_UNKNOWN;

    try {
      NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
      if (activeNetwork == null || !activeNetwork.isConnected())
        return STATE_DISCONNECTED;

      if (activeNetwork.isRoaming())
        return STATE_ROAMING;

      return STATE_CONNECTED;
    } catch (Exception e) {
      /* java.lang.SecurityException can occur if
         android.permission.ACCESS_NETWORK_STATE is not given */
      Log.d(TAG, "ConnectivityManager failed", e);
      return STATE_UNKNOWN;
    }
  }
}
