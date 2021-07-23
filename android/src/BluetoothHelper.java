/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

  private final BluetoothAdapter adapter;

  /**
   * Does this device support Bluetooth Low Energy?
   */
  private final boolean hasLe;

  BluetoothHelper(Context context) throws Exception {
    adapter = BluetoothAdapter.getDefaultAdapter();
    if (adapter == null)
      throw new Exception("No Bluetooth adapter found");

    hasLe = context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE);
  }

  public boolean isEnabled() {
    return adapter.isEnabled();
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

  public String getNameFromAddress(String address) {
    try {
      return adapter.getRemoteDevice(address).getName();
    } catch (Exception e) {
      Log.e(TAG, "Failed to look up name of " + address, e);
      return null;
    }
  }

  public String[] list() {
    Set<BluetoothDevice> devices = adapter.getBondedDevices();
    if (devices == null)
      return null;

    String[] addresses = new String[devices.size() * 3];
    int n = 0;
    for (BluetoothDevice device: devices) {
      addresses[n++] = device.getAddress();
      addresses[n++] = device.getName();
      addresses[n++] = BluetoothDevice.DEVICE_TYPE_LE == device.getType() ? "BLE" : "CLASSIC";
    }

    return addresses;
  }

  public boolean startLeScan(BluetoothAdapter.LeScanCallback cb) {
    return hasLe && adapter.startLeScan(cb);
  }

  public void stopLeScan(BluetoothAdapter.LeScanCallback cb) {
    if (hasLe)
      adapter.stopLeScan(cb);
  }

  public AndroidPort connectHM10(Context context, String address)
    throws IOException {
    if (!hasLe)
      throw new IOException("No Bluetooth adapter found");

    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      throw new IOException("Bluetooth device not found");

    Log.d(TAG, String.format("Bluetooth device \"%s\" (%s) is a LE device, trying to connect using GATT...",
                             device.getName(), device.getAddress()));
    return new HM10Port(context, device);
  }

  public AndroidPort connect(Context context, String address)
    throws IOException {
    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      throw new IOException("Bluetooth device not found");

    BluetoothSocket socket =
      device.createRfcommSocketToServiceRecord(THE_UUID);
    return new BluetoothClientPort(socket);
  }

  public AndroidPort createServer() throws IOException {
    return new BluetoothServerPort(adapter, THE_UUID);
  }
}
