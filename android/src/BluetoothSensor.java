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

import java.io.IOException;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Build;

/**
 * Read Bluetooth LE sensor values and report them to a
 * #SensorListener.
 */
public final class BluetoothSensor
  extends BluetoothGattCallback
  implements AndroidSensor
{
  private final SensorListener listener;

  private final BluetoothGatt gatt;

  private int state = STATE_LIMBO;

  public BluetoothSensor(Context context, BluetoothDevice device,
                         SensorListener listener)
    throws IOException
  {
    this.listener = listener;

    if (Build.VERSION.SDK_INT >= 23)
      gatt = device.connectGatt(context, false, this, BluetoothDevice.TRANSPORT_LE);
    else
      gatt = device.connectGatt(context, false, this);

    if (gatt == null)
      throw new IOException("Bluetooth GATT connect failed");
  }

  @Override
  public void close() {
    gatt.close();
  }

  @Override
  public int getState() {
    return state;
  }

  private void submitError(String msg) {
    state = STATE_FAILED;
    listener.onSensorError(msg);
  }

  private void enableNotification(BluetoothGattCharacteristic c) {
    BluetoothGattDescriptor d = c.getDescriptor(BluetoothUuids.CLIENT_CHARACTERISTIC_CONFIGURATION);
    if (d == null)
      return;

    gatt.setCharacteristicNotification(c, true);
    d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
    gatt.writeDescriptor(d);
  }

  private void readHeartRateMeasurement(BluetoothGattCharacteristic c) {
    int offset = 0;
    final int flags = c.getIntValue(c.FORMAT_UINT8, offset);
    ++offset;

    final boolean bpm16 = (flags & 0x1) != 0;

    final int bpm = bpm16
      ? c.getIntValue(c.FORMAT_UINT16, offset)
      : c.getIntValue(c.FORMAT_UINT8, offset);

    listener.onHeartRateSensor(bpm);
  }

  @Override
  public synchronized void onCharacteristicChanged(BluetoothGatt gatt,
                                                   BluetoothGattCharacteristic c) {
    try {
      if (BluetoothUuids.HEART_RATE_SERVICE.equals(c.getService().getUuid())) {
        if (BluetoothUuids.HEART_RATE_MEASUREMENT_CHARACTERISTIC.equals(c.getUuid())) {
          readHeartRateMeasurement(c);
        }
      }

      if (BluetoothUuids.FLYTEC_SENSBOX_SERVICE.equals(c.getService().getUuid())) {
        if (BluetoothUuids.FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC.equals(c.getUuid())) {
          listener.onLocationSensor(c.getIntValue(c.FORMAT_UINT32, 0),
                                    -1,
                                    c.getIntValue(c.FORMAT_SINT32, 8) / 10000000.,
                                    c.getIntValue(c.FORMAT_SINT32, 4) / 10000000.,
                                    false, 0,
                                    false, 0,
                                    false, 0,
                                    false, 0,
                                    false, 0);

          listener.onPressureAltitudeSensor(c.getIntValue(c.FORMAT_SINT16, 12));
          listener.onVarioSensor(c.getIntValue(c.FORMAT_SINT16, 16) / 10.f);

          //c.getIntValue(c.FORMAT_SINT16, 14), // ??
        }
      }
    } catch (NullPointerException e) {
      /* probably caused by a malformed value - ignore */
    }
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

    BluetoothGattService service = gatt.getService(BluetoothUuids.HEART_RATE_SERVICE);
    if (service != null) {
      BluetoothGattCharacteristic c =
        service.getCharacteristic(BluetoothUuids.HEART_RATE_MEASUREMENT_CHARACTERISTIC);
      if (c != null) {
        state = STATE_READY;
        enableNotification(c);
      }
    }

    /* enable notifications for Flytec Sensbox */
    service = gatt.getService(BluetoothUuids.FLYTEC_SENSBOX_SERVICE);
    if (service != null) {
      BluetoothGattCharacteristic c =
        service.getCharacteristic(BluetoothUuids.FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC);
      if (c != null)
        enableNotification(c);

      c = service.getCharacteristic(BluetoothUuids.FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC);
      if (c != null) {
        state = STATE_READY;
        enableNotification(c);
      }
    }

    if (state == STATE_LIMBO)
      submitError("Unsupported Bluetooth device");
  }
}
