#pragma once

#include <array>
#include <string_view>

namespace BluetoothUuids {

constexpr std::string_view GENERIC_ACCESS_SERVICE = "00001800-0000-1000-8000-00805F9B34FB";
constexpr std::string_view CLIENT_CHARACTERISTIC_CONFIGURATION = "00002902-0000-1000-8000-00805f9b34fb";
constexpr std::string_view DEVICE_NAME_CHARACTERISTIC = "00002A00-0000-1000-8000-00805F9B34FB";
constexpr std::string_view HEART_RATE_SERVICE = "0000180D-0000-1000-8000-00805F9B34FB";
constexpr std::string_view HEART_RATE_MEASUREMENT_CHARACTERISTIC = "00002A37-0000-1000-8000-00805F9B34FB";

/**
 * @see https://sites.google.com/view/ppgmeter/startpage
 * Engine sensors service and characteristic
 */
constexpr std::string_view ENGINE_SENSORS_SERVICE = "D2865ECA-2C07-4610-BF03-8AEEBEF047FB";
constexpr std::string_view ENGINE_SENSORS_CHARACTERISTIC = "D2865ECB-2C07-4610-BF03-8AEEBEF047FB";

constexpr std::string_view HM10_SERVICE = "0000FFE0-0000-1000-8000-00805F9B34FB";

/**
 * The HM-10 and compatible bluetooth modules use a GATT characteristic
 * with this UUID for sending and receiving data.
 */
constexpr std::string_view HM10_RX_TX_CHARACTERISTIC = "0000FFE1-0000-1000-8000-00805F9B34FB";

/* Flytec Sensbox */
constexpr std::string_view FLYTEC_SENSBOX_SERVICE = "aba27100-143b-4b81-a444-edcd0000f020";

/**
 * @see https://github.com/flytec/SensBoxLib_iOS/blob/master/_SensBox%20Documentation/SensorBox%20BLE%20Protocol.pdf
 */
constexpr std::string_view FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC = "aba27100-143b-4b81-a444-edcd0000f022";
constexpr std::string_view FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC = "aba27100-143b-4b81-a444-edcd0000f023";
constexpr std::string_view FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC = "aba27100-143b-4b81-a444-edcd0000f024";
constexpr std::string_view FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC = "aba27100-143b-4b81-a444-edcd0000f025";


// Helper method returning services in an array
constexpr std::array<std::string_view, 5>
getAllServiceUuids()
{
  return {
    GENERIC_ACCESS_SERVICE,
    HEART_RATE_SERVICE,
    ENGINE_SENSORS_SERVICE,
    HM10_SERVICE,
    FLYTEC_SENSBOX_SERVICE
  };
}

// Helper method returning characteristics in an array
constexpr std::array<std::string_view, 9>
getAllCharacteristicsUuids()
{
  return {
    CLIENT_CHARACTERISTIC_CONFIGURATION,
    DEVICE_NAME_CHARACTERISTIC,
    HEART_RATE_MEASUREMENT_CHARACTERISTIC,
    ENGINE_SENSORS_CHARACTERISTIC,
    HM10_RX_TX_CHARACTERISTIC,
    FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC,
    FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC,
    FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC,
    FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC
  };
}

} // namespace BluetoothUuids
