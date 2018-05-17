/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
