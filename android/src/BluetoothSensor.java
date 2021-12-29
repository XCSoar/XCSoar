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

import java.util.Queue;
import java.util.LinkedList;
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
  private final SafeDestruct safeDestruct = new SafeDestruct();

  private final BluetoothGatt gatt;

  private int state = STATE_LIMBO;

  private BluetoothGattCharacteristic currentEnableNotification;
  private final Queue<BluetoothGattCharacteristic> enableNotificationQueue =
    new LinkedList<BluetoothGattCharacteristic>();

  private boolean haveFlytecMovement = false;
  private double flytecGroundSpeed, flytecTrack;
  private int flytecSatellites = 0;

  public BluetoothSensor(Context context, BluetoothDevice device,
                         SensorListener listener)
    throws IOException
  {
    this.listener = listener;

    if (Build.VERSION.SDK_INT >= 23)
      gatt = device.connectGatt(context, true, this, BluetoothDevice.TRANSPORT_LE);
    else
      gatt = device.connectGatt(context, true, this);

    if (gatt == null)
      throw new IOException("Bluetooth GATT connect failed");
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();
    gatt.close();
    safeDestruct.finishShutdown();
  }

  @Override
  public int getState() {
    return state;
  }

  private void setStateSafe(int _state) {
    if (_state == state)
      return;

    state = _state;

    if (safeDestruct.increment()) {
      try {
        listener.onSensorStateChanged();
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  private void submitError(String msg) {
    haveFlytecMovement = false;
    flytecSatellites = 0;
    state = STATE_FAILED;

    if (safeDestruct.increment()) {
      try {
        listener.onSensorError(msg);
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  private boolean doEnableNotification(BluetoothGattCharacteristic c) {
    BluetoothGattDescriptor d = c.getDescriptor(BluetoothUuids.CLIENT_CHARACTERISTIC_CONFIGURATION);
    if (d == null)
      return false;

    gatt.setCharacteristicNotification(c, true);
    d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
    return gatt.writeDescriptor(d);
  }

  private void enableNotification(BluetoothGattCharacteristic c) {
    synchronized(enableNotificationQueue) {
      if (currentEnableNotification == null) {
        currentEnableNotification = c;
        if (!doEnableNotification(c))
          currentEnableNotification = null;
      } else
        enableNotificationQueue.add(c);
    }
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

  static long toUnsignedLong(int x) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
      // Android 7 "Nougat" supports Java 8
      return Integer.toUnsignedLong(x);

    // Reimplement for older Android versions
    long l = x;
    if (l < 0)
      l += (1L << 32);
    return l;
  }

  @Override
  public synchronized void onCharacteristicChanged(BluetoothGatt gatt,
                                                   BluetoothGattCharacteristic c) {
    if (!safeDestruct.increment())
      return;

    try {
      if (BluetoothUuids.HEART_RATE_SERVICE.equals(c.getService().getUuid())) {
        if (BluetoothUuids.HEART_RATE_MEASUREMENT_CHARACTERISTIC.equals(c.getUuid())) {
          readHeartRateMeasurement(c);
        }
      }

      if (BluetoothUuids.FLYTEC_SENSBOX_SERVICE.equals(c.getService().getUuid())) {
        if (BluetoothUuids.FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC.equals(c.getUuid())) {
          /* protocol documentation:
             https://github.com/flytec/SensBoxLib_iOS/blob/master/_SensBox%20Documentation/SensorBox%20BLE%20Protocol.pdf */
          final int gps_status = c.getIntValue(c.FORMAT_UINT8, 18) & 0x7;
          final boolean hasAltitude = (gps_status == 2 || gps_status == 4);

          final long time = 1000 *
            toUnsignedLong(c.getIntValue(c.FORMAT_UINT32, 0));

          listener.onLocationSensor(time,
                                    flytecSatellites,
                                    c.getIntValue(c.FORMAT_SINT32, 8) / 10000000.,
                                    c.getIntValue(c.FORMAT_SINT32, 4) / 10000000.,
                                    hasAltitude, true,
                                    c.getIntValue(c.FORMAT_SINT16, 12),
                                    haveFlytecMovement, flytecTrack,
                                    haveFlytecMovement, flytecGroundSpeed,
                                    false, 0);

          listener.onPressureAltitudeSensor(c.getIntValue(c.FORMAT_SINT16, 14));
        } else if (BluetoothUuids.FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC.equals(c.getUuid())) {
          flytecGroundSpeed = c.getIntValue(c.FORMAT_SINT16, 6) / 10.;
          flytecTrack = c.getIntValue(c.FORMAT_SINT16, 8) / 10.;

          listener.onVarioSensor(c.getIntValue(c.FORMAT_SINT16, 4) / 100.f);
          listener.onAccelerationSensor1(c.getIntValue(c.FORMAT_UINT16, 16) / 10.);

          haveFlytecMovement = true;
        } else if (BluetoothUuids.FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC.equals(c.getUuid())) {
          flytecSatellites = c.getIntValue(c.FORMAT_UINT8, 6);
        } else if (BluetoothUuids.FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC.equals(c.getUuid())) {
          listener.onBatteryPercent(c.getIntValue(c.FORMAT_UINT8, 4));

          final double CELSIUS_OFFSET = 273.15;
          double temperatureCelsius = c.getIntValue(c.FORMAT_SINT16, 6) / 10.;
          listener.onTemperature(CELSIUS_OFFSET + temperatureCelsius);
        }
      }
    } catch (NullPointerException e) {
      /* probably caused by a malformed value - ignore */
    } finally {
      safeDestruct.decrement();
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
  public void onDescriptorWrite(BluetoothGatt gatt,
                                BluetoothGattDescriptor descriptor,
                                int status) {
    synchronized(enableNotificationQueue) {
      currentEnableNotification = enableNotificationQueue.poll();
      if (currentEnableNotification != null) {
        if (!doEnableNotification(currentEnableNotification))
          currentEnableNotification = null;
      }
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
        setStateSafe(STATE_READY);
        enableNotification(c);
      }
    }

    /* enable notifications for Flytec Sensbox */
    service = gatt.getService(BluetoothUuids.FLYTEC_SENSBOX_SERVICE);
    if (service != null) {
      BluetoothGattCharacteristic c =
        service.getCharacteristic(BluetoothUuids.FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC);
      if (c != null) {
        setStateSafe(STATE_READY);
        enableNotification(c);
      }

      c = service.getCharacteristic(BluetoothUuids.FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC);
      if (c != null)
        enableNotification(c);

      c = service.getCharacteristic(BluetoothUuids.FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC);
      if (c != null)
        enableNotification(c);

      c = service.getCharacteristic(BluetoothUuids.FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC);
      if (c != null)
        enableNotification(c);
    }

    if (state == STATE_LIMBO)
      submitError("Unsupported Bluetooth device");
  }
}
