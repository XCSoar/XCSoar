// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.util.Log;

import java.net.Inet4Address;

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

  public static String getWifiIpAddress() {
    if (cm == null)
      return null;

    try {
      Network network = cm.getActiveNetwork();
      if (network == null)
        return null;

      NetworkCapabilities capabilities = cm.getNetworkCapabilities(network);
      if (capabilities == null)
        return null;

      // Only return IP if this is a WiFi network
      if (!capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI))
        return null;

      LinkProperties properties = cm.getLinkProperties(network);
      if (properties == null)
        return null;

      for (LinkAddress address : properties.getLinkAddresses()) {
        if (address.getAddress() instanceof Inet4Address &&
            !address.getAddress().isLoopbackAddress())
          return address.getAddress().getHostAddress();
      }
    } catch (Exception e) {
      Log.d(TAG, "Failed to query WiFi IP address", e);
    }

    return null;
  }
}
