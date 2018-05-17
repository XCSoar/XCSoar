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

import android.util.Log;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.pm.PackageManager;

/**
 * A library that constructs Bluetooth ports.  It is called by C++
 * code.
 */
final class BluetoothHelper {
  private static final String TAG = "XCSoar";
  private static final UUID THE_UUID =
        UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

  private static final BluetoothAdapter adapter;

  /**
   * Does this device support Bluetooth Low Energy?
   */
  private static boolean hasLe;

  static {
    BluetoothAdapter _adapter;
    try {
      _adapter = BluetoothAdapter.getDefaultAdapter();
    } catch (Exception e) {
      Log.e(TAG, "BluetoothAdapter.getDefaultAdapter() failed", e);
      _adapter = null;
    }

    adapter = _adapter;
  }

  public static void Initialize(Context context) {
    hasLe = adapter != null && android.os.Build.VERSION.SDK_INT >= 18 &&
      context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE);
  }

  public static boolean isEnabled() {
    return adapter != null && adapter.isEnabled();
  }

  /**
   * Turns the #BluetoothDevice into a human-readable string.
   */
  public static String getDisplayString(BluetoothDevice device) {
    String name = device.getName();
    String address = device.getAddress();

    if (name == null)
      return address;

    return name + " [" + address + "]";
  }

  public static String getDisplayString(BluetoothSocket socket) {
    return getDisplayString(socket.getRemoteDevice());
  }

  public static String getNameFromAddress(String address) {
    if (adapter == null)
      return null;

    try {
      return adapter.getRemoteDevice(address).getName();
    } catch (Exception e) {
      Log.e(TAG, "Failed to look up name of " + address, e);
      return null;
    }
  }

  public static String[] list() {
    if (adapter == null)
      return null;

    Set<BluetoothDevice> devices = adapter.getBondedDevices();
    if (devices == null)
      return null;

    String[] addresses = new String[devices.size() * 2];
    int n = 0;
    for (BluetoothDevice device: devices) {
      addresses[n++] = device.getAddress();
      addresses[n++] = device.getName();
    }

    return addresses;
  }

  public static boolean startLeScan(BluetoothAdapter.LeScanCallback cb) {
    return hasLe && adapter.startLeScan(cb);
  }

  public static void stopLeScan(BluetoothAdapter.LeScanCallback cb) {
    if (hasLe)
      adapter.stopLeScan(cb);
  }

  public static AndroidPort connect(Context context, String address)
    throws IOException {
    if (adapter == null)
      return null;

    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      return null;

    if (hasLe && BluetoothDevice.DEVICE_TYPE_LE == device.getType()) {
      Log.d(TAG, String.format(
                               "Bluetooth device \"%s\" (%s) is a LE device, trying to connect using GATT...",
                               device.getName(), device.getAddress()));
      BluetoothGattClientPort gattClientPort
        = new BluetoothGattClientPort(device);
      gattClientPort.startConnect(context);
      return gattClientPort;
    } else {
      BluetoothSocket socket =
        device.createRfcommSocketToServiceRecord(THE_UUID);
      return new BluetoothClientPort(socket);
    }
  }

  public static AndroidPort createServer() throws IOException {
    if (adapter == null)
      return null;

    return new BluetoothServerPort(adapter, THE_UUID);
  }
}
