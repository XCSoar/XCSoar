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
