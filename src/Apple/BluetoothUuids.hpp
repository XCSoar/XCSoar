// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#import <CoreBluetooth/CoreBluetooth.h>

/**
 * Various Bluetooth service/characteristic UUIDs.
 *
 * Keep this in sync with android/src/BluetoothUuids.java, the
 * Android counterpart of this namespace: the constants use the same
 * names and values.  Some of them are not used on iOS (yet); the
 * comment next to each of those explains why.
 */
namespace BluetoothUuids {

/**
 * Not used on iOS: CoreBluetooth provides the device name as
 * CBPeripheral.name, the Generic Access service need not be read.
 */
static constexpr char GENERIC_ACCESS_SERVICE[] =
  "00001800-0000-1000-8000-00805F9B34FB";

/**
 * Not used on iOS: -[CBPeripheral setNotifyValue:forCharacteristic:]
 * writes this descriptor by itself.
 */
static constexpr char CLIENT_CHARACTERISTIC_CONFIGURATION[] =
  "00002902-0000-1000-8000-00805f9b34fb";

/**
 * Not used on iOS: see GENERIC_ACCESS_SERVICE.
 */
static constexpr char DEVICE_NAME_CHARACTERISTIC[] =
  "00002A00-0000-1000-8000-00805F9B34FB";

static constexpr char HEART_RATE_SERVICE[] =
  "0000180D-0000-1000-8000-00805F9B34FB";

/**
 * Not used on iOS: BLE sensors are not supported (yet), only serial
 * port bridges; the service UUID above is only used to derive the
 * feature flags for the port picker.
 */
static constexpr char HEART_RATE_MEASUREMENT_CHARACTERISTIC[] =
  "00002A37-0000-1000-8000-00805F9B34FB";

/**
 * @see https://sites.google.com/view/ppgmeter/startpage
 * Engine sensors service and characteristic
 *
 * Not used on iOS: BLE sensors are not supported (yet).
 */
static constexpr char ENGINE_SENSORS_SERVICE[] =
  "D2865ECA-2C07-4610-BF03-8AEEBEF047FB";
static constexpr char ENGINE_SENSORS_CHARACTERISTIC[] =
  "D2865ECB-2C07-4610-BF03-8AEEBEF047FB";

static constexpr char HM10_SERVICE[] =
  "0000FFE0-0000-1000-8000-00805F9B34FB";

/**
 * The HM-10 and compatible bluetooth modules use a GATT characteristic
 * with this UUID for sending and receiving data.
 */
static constexpr char HM10_RX_TX_CHARACTERISTIC[] =
  "0000FFE1-0000-1000-8000-00805F9B34FB";

/**
 * Nordic UART Service (NUS) - provides a BLE serial bridge using
 * separate RX and TX characteristics.
 */
static constexpr char NORDIC_UART_SERVICE[] =
  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";

/**
 * Nordic UART RX characteristic - XCSoar writes data to it.
 */
static constexpr char NORDIC_UART_RX_CHARACTERISTIC[] =
  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";

/**
 * Nordic UART TX characteristic - XCSoar receives data from here.
 */
static constexpr char NORDIC_UART_TX_CHARACTERISTIC[] =
  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

/**
 * Microchip/ISSC transparent UART service (e.g. BlueFly Vario BLE).
 */
static constexpr char ISSC_UART_SERVICE[] =
  "49535343-FE7D-4AE5-8FA9-9FAFD205E455";

/**
 * ISSC UART RX characteristic - XCSoar writes data to it
 * (Write Without Response).
 */
static constexpr char ISSC_UART_RX_CHARACTERISTIC[] =
  "49535343-8841-43F4-A8D4-ECBE34729BB3";

/**
 * ISSC UART TX characteristic - XCSoar receives data from here
 * (Notify).
 */
static constexpr char ISSC_UART_TX_CHARACTERISTIC[] =
  "49535343-1E4D-4BD9-BA61-23C647249616";

/* Flytec Sensbox */
static constexpr char FLYTEC_SENSBOX_SERVICE[] =
  "aba27100-143b-4b81-a444-edcd0000f020";

/**
 * @see https://github.com/flytec/SensBoxLib_iOS/blob/master/_SensBox%20Documentation/SensorBox%20BLE%20Protocol.pdf
 *
 * Not used on iOS: BLE sensors are not supported (yet); the service
 * UUID above is only used to derive the feature flags for the port
 * picker.
 */
static constexpr char FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC[] =
  "aba27100-143b-4b81-a444-edcd0000f022";
static constexpr char FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC[] =
  "aba27100-143b-4b81-a444-edcd0000f023";
static constexpr char FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC[] =
  "aba27100-143b-4b81-a444-edcd0000f024";
static constexpr char FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC[] =
  "aba27100-143b-4b81-a444-edcd0000f025";

/**
 * Convert one of the constants above to a #CBUUID.  The object is
 * created on first use and then cached, because the scan callback
 * compares UUIDs many times per second.
 */
template<const char *uuid>
[[gnu::pure]]
static inline CBUUID *
Uuid() noexcept
{
  static CBUUID *const instance = [CBUUID UUIDWithString:@(uuid)];
  return instance;
}

/**
 * All service UUIDs which provide a serial port emulation.
 *
 * Unlike Android's getAllServiceUuids(), this is not a scan filter:
 * iOS scans without one, because many UART bridges do not advertise
 * their service UUID.  It is only used to find bridges which are
 * already connected at the system level.  There is no counterpart
 * of getAllCharacteristicsUuids(), which Android needs for BLE
 * sensors only.
 */
[[gnu::pure]]
static inline NSArray<CBUUID *> *
SerialServiceUuids() noexcept
{
  static NSArray<CBUUID *> *const uuids = @[
    Uuid<HM10_SERVICE>(),
    Uuid<NORDIC_UART_SERVICE>(),
    Uuid<ISSC_UART_SERVICE>(),
  ];
  return uuids;
}

} // namespace BluetoothUuids
