/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

import java.util.Arrays;
import java.util.UUID;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.os.Build;
import android.util.Log;

/**
 * AndroidPort implementation for Bluetooth Low Energy devices using the
 * GATT protocol.
 */
public class BluetoothGattClientPort
    extends BluetoothGattCallback
    implements AndroidPort  {
  private static final String TAG = "XCSoar";

  /**
   * The HM-10 and compatible bluetooth modules use a GATT characteristic
   * with this UUID for sending and receiving data.
   */
  private static final UUID RX_TX_CHARACTERISTIC_UUID =
      UUID.fromString("0000FFE1-0000-1000-8000-00805F9B34FB");

  private BluetoothGatt gatt;
  private InputListener listener;
  private BluetoothGattCharacteristic dataCharacteristic;
  private boolean writePending = false;  
  private int portState = STATE_LIMBO;

  public void startConnect(BluetoothGatt _gatt) {
    gatt = _gatt;
    gatt.connect();
  }

  private BluetoothGattCharacteristic findDataCharacteristic() {
    try {
      for (BluetoothGattService gattService : gatt.getServices()) {
        for (BluetoothGattCharacteristic characteristic :
            gattService.getCharacteristics()) {
          if (RX_TX_CHARACTERISTIC_UUID.equals(characteristic.getUuid())) {
            return characteristic;
          }
        }
      }
      Log.e(TAG, "GATT data characteristic not found");
      return null;
    } catch (Exception e) {
      Log.e(TAG, "GATT data characteristic lookup failed", e);
      return null;
    }
  }

  @Override
  public synchronized void onConnectionStateChange(BluetoothGatt _gatt, int status,
      int newState) {
    int newPortState = STATE_LIMBO;
    if (BluetoothProfile.STATE_CONNECTED == newState) {
      if (!gatt.discoverServices()) {
        Log.e(TAG, "Discovering GATT services request failed");
        newPortState = STATE_FAILED;
      }
    } else {
      dataCharacteristic = null;
      if (BluetoothProfile.STATE_DISCONNECTED == newState) {
        Log.d(TAG, "Received GATT disconnected event");
        newPortState = STATE_FAILED;
      }
    }
    writePending = false;
    portState = newPortState;
  }

  @Override
  public synchronized void onServicesDiscovered(BluetoothGatt gatt,
      int status) {
    if (BluetoothGatt.GATT_SUCCESS == status) {
      BluetoothGattCharacteristic newDataCharacteristic = findDataCharacteristic();
      if (null == newDataCharacteristic) {
        portState = STATE_FAILED;
      } else {
        if (gatt.setCharacteristicNotification(newDataCharacteristic, true)) {
          dataCharacteristic = newDataCharacteristic;
          portState = STATE_READY;
        } else {
          Log.e(TAG, "Could not enable GATT characteristic notification");
          portState = STATE_FAILED;
        }
      }
    } else {
      Log.e(TAG, "Discovering GATT services failed");
      portState = STATE_FAILED;
    }
  }

  @Override
  public synchronized void onCharacteristicWrite(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    if (BluetoothGatt.GATT_SUCCESS != status) {
      Log.e(TAG, "GATT characteristic write failed");
    }
    writePending = false;
    notifyAll();
  }

  @Override
  public void onCharacteristicChanged(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic) {
    if ((dataCharacteristic != null) &&
        (dataCharacteristic.getUuid().equals(characteristic.getUuid()))) {
      synchronized (this) {
        if (listener != null) {
          byte[] data = characteristic.getValue();
          listener.dataReceived(data, data.length);;
        }
      }
    }
  }

  @Override
  public synchronized void setListener(InputListener _listener) {
    listener = _listener;
  }

  @Override
  public void close() {
    gatt.close();
  }

  @Override
  public int getState() {
    return portState;
  }

  @Override
  public synchronized boolean drain() {
    while (writePending) {
      try {
        wait();
      } catch (InterruptedException e) {
        return false;
      }
    }
    return true;
  }

  @Override
  public int getBaudRate() {
    return 0;
  }

  @Override
  public boolean setBaudRate(int baud) {
    return true;
  }

  @Override
  public synchronized int write(byte[] data, int length) {
    if (0 == length) return 0;
    if (dataCharacteristic == null) return 0;
    if (writePending && !drain()) return 0;
    dataCharacteristic.setValue(Arrays.copyOf(data, length));
    writePending = true;
    if (!gatt.writeCharacteristic(dataCharacteristic)) {
      Log.e(TAG, "GATT characteristic write request failed");
      return 0;
    }
    return length;
  }
}
