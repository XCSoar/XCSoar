// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/GPSDeviceName.hpp"
#include "Device/Config.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"

#include <array>

static DeviceConfig
MakeDevice(DeviceConfig::PortType port_type,
           const char *driver_name = "") noexcept
{
  DeviceConfig device{};
  device.port_type = port_type;
  device.driver_name = driver_name;
  return device;
}

static NMEAInfo
MakeData(bool alive, bool location) noexcept
{
  NMEAInfo basic;
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{1}};
  if (alive)
    basic.alive.Update(basic.clock);
  if (location)
    basic.location_available.Update(basic.clock);
  return basic;
}

/**
 * HFGPS names the device that supplies the fix, by the rule of
 * DeviceBlackboard::Merge() (#3180).
 */
static void
TestFindGPSDevice() noexcept
{
  const NMEAInfo none = MakeData(false, false);
  const NMEAInfo no_fix = MakeData(true, false);
  const NMEAInfo fix = MakeData(true, true);
  const NMEAInfo stale_fix = MakeData(false, true);

  /* the GPS is on device B, device A is configured but silent */
  std::array<const NMEAInfo *, 3> b_only{&none, &fix, &none};
  ok1(FindGPSDevice(b_only) == 1);

  /* device A is alive but has no fix: still B */
  std::array<const NMEAInfo *, 3> a_no_fix{&no_fix, &fix, &none};
  ok1(FindGPSDevice(a_no_fix) == 1);

  /* both have a fix: the first one wins, as in the merge */
  std::array<const NMEAInfo *, 3> both{&none, &fix, &fix};
  ok1(FindGPSDevice(both) == 1);

  /* a device that is not alive does not count, whatever it last had */
  std::array<const NMEAInfo *, 3> stale{&stale_fix, &none, &fix};
  ok1(FindGPSDevice(stale) == 2);

  /* nothing yet: no device; the header falls back to device A */
  std::array<const NMEAInfo *, 3> nothing{&none, &no_fix, &none};
  ok1(!FindGPSDevice(nothing).has_value());
}

int main()
{
  plan_tests(6 + 5);

  TestFindGPSDevice();

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
