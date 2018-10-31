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

import java.util.UUID;
import java.util.Set;
import java.io.IOException;

import android.content.SharedPreferences;
import android.util.Log;
import android.bluetooth.BluetoothDevice;
import android.content.Context;

/**
 * A cache for Bluetooth LE device type and names.
 */

final class BluetoothCache {
  private static final String TAG = "XCSoar";

  private static final String PREF_FILE = "btdevices";
  private static final String TYPE_PREFIX = "type_";
  private static final String NAME_PREFIX = "name_";
  private static SharedPreferences prefs = null;

  /**
   * Initialise the SharedPrefs object.
   * @param context The application context, required to access resources.
   */

  public static void initialize(Context context) {
    prefs = context.getSharedPreferences(PREF_FILE, 0);
  }

  /**
   * Add a device to the cache. Overwrite any existing entry
   * @param device The Bluetooth LE device to add to the cache
   */

  public static void addDevice(BluetoothDevice device) {
    if(prefs == null)
      return;
    SharedPreferences.Editor editor = prefs.edit();
    editor.putBoolean(TYPE_PREFIX + device.getAddress().toUpperCase(),
      BluetoothDevice.DEVICE_TYPE_LE == device.getType());
    editor.putString(NAME_PREFIX + device.getAddress().toUpperCase(), device.getName());
    editor.apply();
  }

  /**
   * Get the name of the device with the supplied address or null if not known
   * @param address The mac address of the device
   * @return The cached device name
   */

  public static String getName(String address) {
    address = address.toUpperCase();
    return prefs.getString(NAME_PREFIX + address, null);
  }

  /**
   * Is the device with the given address a BLE device?
   * @param address The mac address of the device
   * @return true if the device supports BLE
   */

  public static boolean isBle(String address) {
    return prefs.getBoolean(TYPE_PREFIX + address.toUpperCase(), false);
  }
}
