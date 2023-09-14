// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.UUID;

/**
 * Various Bluetooth service/characteristic UUIDs.
 */
public interface BluetoothUuids {
  UUID GENERIC_ACCESS_SERVICE =
    UUID.fromString("00001800-0000-1000-8000-00805F9B34FB");

  UUID CLIENT_CHARACTERISTIC_CONFIGURATION =
    UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

  UUID DEVICE_NAME_CHARACTERISTIC =
    UUID.fromString("00002A00-0000-1000-8000-00805F9B34FB");

  UUID HEART_RATE_SERVICE =
    UUID.fromString("0000180D-0000-1000-8000-00805F9B34FB");

  UUID HEART_RATE_MEASUREMENT_CHARACTERISTIC =
    UUID.fromString("00002A37-0000-1000-8000-00805F9B34FB");

  /**
   * @see https://sites.google.com/view/ppgmeter/startpage
   * Engine sensors service and characteristic
   */
  UUID ENGINE_SENSORS_SERVICE =
    UUID.fromString("D2865ECA-2C07-4610-BF03-8AEEBEF047FB");
  UUID ENGINE_SENSORS_CHARACTERISTIC =
    UUID.fromString("D2865ECB-2C07-4610-BF03-8AEEBEF047FB");

  UUID HM10_SERVICE =
    UUID.fromString("0000FFE0-0000-1000-8000-00805F9B34FB");

  /**
   * The HM-10 and compatible bluetooth modules use a GATT characteristic
   * with this UUID for sending and receiving data.
   */
  UUID HM10_RX_TX_CHARACTERISTIC =
    UUID.fromString("0000FFE1-0000-1000-8000-00805F9B34FB");

  /* Flytec Sensbox */
  UUID FLYTEC_SENSBOX_SERVICE =
    UUID.fromString("aba27100-143b-4b81-a444-edcd0000f020");

  /**
   * @see https://github.com/flytec/SensBoxLib_iOS/blob/master/_SensBox%20Documentation/SensorBox%20BLE%20Protocol.pdf
   */
  UUID FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC =
    UUID.fromString("aba27100-143b-4b81-a444-edcd0000f022");
  UUID FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC =
    UUID.fromString("aba27100-143b-4b81-a444-edcd0000f023");
  UUID FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC =
    UUID.fromString("aba27100-143b-4b81-a444-edcd0000f024");
  UUID FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC =
    UUID.fromString("aba27100-143b-4b81-a444-edcd0000f025");
}
