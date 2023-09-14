// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  /**
   * Data in the characteristic has little endian byteorder.
   * Lowest bit of flags indicates valid ignitions_per_sec reading.
   * 0 Kelvin indicates invalid temperatures e.g 
   * no temperature sensor present.
  */
  private void engineSensorDataToListeners(BluetoothGattCharacteristic c) {
    final int flags = c.getIntValue(c.FORMAT_UINT8, 0);
    final int cht_temp = c.getIntValue(c.FORMAT_UINT16, 1);
    final int egt_temp = c.getIntValue(c.FORMAT_UINT16, 3);
    final int outside_air_temperature = c.getIntValue(c.FORMAT_UINT16, 5);

    if(outside_air_temperature != 0)
      listener.onTemperature(outside_air_temperature);

    final int pressure = c.getIntValue(c.FORMAT_UINT32, 7);

    // Just guessing the sensor_noise_variance.
    if(pressure != 0)
      listener.onBarometricPressureSensor(pressure / 100.0f, 0.01f);

    final int ignitions_per_second = c.getIntValue(c.FORMAT_UINT16, 11);
    listener.onEngineSensors(cht_temp != 0 ? true : false,
                             cht_temp,
                             egt_temp != 0 ? true : false,
                             egt_temp,
                             (flags&0x01) == 0x01 ? true : false,
                             ignitions_per_second);
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

      if (BluetoothUuids.ENGINE_SENSORS_SERVICE.equals(c.getService().getUuid()) &&
          BluetoothUuids.ENGINE_SENSORS_CHARACTERISTIC.equals(c.getUuid())) {
        engineSensorDataToListeners(c);
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

    /* enable notifications for DR Solutions engine sensor box */
    service = gatt.getService(BluetoothUuids.ENGINE_SENSORS_SERVICE);
    if (service != null) {
      BluetoothGattCharacteristic c =
        service.getCharacteristic(BluetoothUuids.ENGINE_SENSORS_CHARACTERISTIC);
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
