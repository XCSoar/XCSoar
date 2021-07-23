/* Copyright_License {

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

import java.util.List;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Build;
import android.util.Log;

/**
 * Detect features of a Bluetooth LE GATT device.
 */
public class BluetoothIdentify extends BluetoothGattCallback {
  private static final String TAG = "XCSoar";

  public static interface Listener {
    void onBluetoothIdentifySuccess(BluetoothDevice device, long features);
    void onBluetoothIdentifyError(BluetoothDevice device, String msg);
  }

  private final BluetoothGatt gatt;
  private Listener listener;
  private volatile boolean shutdown = false;

  public BluetoothIdentify(Context context, BluetoothDevice device,
                           Listener listener) {
    this.listener = listener;

    if (Build.VERSION.SDK_INT >= 23)
      gatt = device.connectGatt(context, false, this,
                                BluetoothDevice.TRANSPORT_LE);
    else
      gatt = device.connectGatt(context, false, this);

    if (gatt == null)
      listener.onBluetoothIdentifyError(device,
                                        "Bluetooth GATT connect failed");
  }

  public synchronized void close() {
    listener = null;
    gatt.close();
  }

  private synchronized Listener consumeListener() {
    Listener l = listener;
    listener = null;
    return l;
  }

  private void submitSuccess(long features) {
    BluetoothDevice device = gatt.getDevice();
    gatt.close();

    Listener l = consumeListener();
    if (l != null)
      l.onBluetoothIdentifySuccess(device, features);
  }

  private void submitError(String msg) {
    BluetoothDevice device = gatt.getDevice();
    gatt.close();

    Listener l = consumeListener();
    if (l != null)
      l.onBluetoothIdentifyError(device, msg);
  }

  @Override
  public void onConnectionStateChange(BluetoothGatt gatt,
                                      int status, int newState) {
    if (BluetoothProfile.STATE_CONNECTED == newState) {
      if (!gatt.discoverServices()) {
        submitError("Discovering GATT services request failed");
      }
    } else {
      submitError("GATT disconnected");
    }
  }

  @Override
  public void onServicesDiscovered(BluetoothGatt gatt,
                                   int status) {
    if (BluetoothGatt.GATT_SUCCESS != status) {
      submitError("Discovering GATT services failed");
      return;
    }

    long features = 0;

    BluetoothGattService service = gatt.getService(BluetoothUuids.HM10_SERVICE);
    if (service != null &&
        service.getCharacteristic(BluetoothUuids.HM10_RX_TX_CHARACTERISTIC) != null)
      features |= DetectDeviceListener.FEATURE_HM10;

    service = gatt.getService(BluetoothUuids.HEART_RATE_SERVICE);
    if (service != null &&
        service.getCharacteristic(BluetoothUuids.HEART_RATE_MEASUREMENT_CHARACTERISTIC) != null)
      features |= DetectDeviceListener.FEATURE_HEART_RATE;

    submitSuccess(features);
  }
}
