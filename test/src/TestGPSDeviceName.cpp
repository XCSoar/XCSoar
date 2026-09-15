// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/GPSDeviceName.hpp"
#include "Device/Config.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"

static DeviceConfig
MakeDevice(DeviceConfig::PortType port_type,
           const char *driver_name = "") noexcept
{
  DeviceConfig device{};
  device.port_type = port_type;
  device.driver_name = driver_name;
  return device;
}

int main()
{
  plan_tests(6);

  const auto serial_flarm = MakeDevice(DeviceConfig::PortType::SERIAL,
                                       "FLARM");
  ok1(StringIsEqual(GetGPSDeviceName(serial_flarm, true), "Simulator"));
  ok1(StringIsEqual(GetGPSDeviceName(serial_flarm, false), "FLARM"));

  const auto disabled = MakeDevice(DeviceConfig::PortType::DISABLED);
  ok1(StringIsEqual(GetGPSDeviceName(disabled, false), "Unknown"));

  const auto ble_sensor = MakeDevice(DeviceConfig::PortType::BLE_SENSOR);
  ok1(StringIsEqual(GetGPSDeviceName(ble_sensor, false), "Unknown"));

  const auto tcp_lx = MakeDevice(DeviceConfig::PortType::TCP_CLIENT, "LX");
  ok1(StringIsEqual(GetGPSDeviceName(tcp_lx, false), "LX"));

  const auto internal = MakeDevice(DeviceConfig::PortType::INTERNAL);
#ifdef ANDROID
  ok1(StringIsEqual(GetGPSDeviceName(internal, false),
                    "Internal GPS (Android)"));
#else
  ok1(StringIsEqual(GetGPSDeviceName(internal, false), "Unknown"));
#endif

  return exit_status();
}
