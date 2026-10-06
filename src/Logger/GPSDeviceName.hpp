// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Config.hpp"
#include "NMEA/Info.hpp"

#include <optional>
#include <span>

/**
 * IGC HFGPS header text for the GPS that is recording the flight.
 * A pure function of the primary device config and whether XCSoar is
 * running as the simulator.
 */
[[gnu::pure]]
inline const char *
GetGPSDeviceName(const DeviceConfig &device, bool simulator) noexcept
{
  if (simulator)
    return "Simulator";

  if (device.UsesDriver())
    return device.driver_name;

  if (device.IsAndroidInternalGPS())
    return "Internal GPS (Android)";

  return "Unknown";
}

/**
 * The device whose GPS fix the merged data uses: the first live device
 * with a location, in order A to H, as DeviceBlackboard::Merge() picks
 * it.  Nothing if none supplies one.
 */
[[gnu::pure]]
inline std::optional<unsigned>
FindGPSDevice(std::span<const NMEAInfo *const> devices) noexcept
{
  for (unsigned i = 0; i < devices.size(); ++i)
    if (devices[i]->alive && devices[i]->location_available)
      return i;

  return std::nullopt;
}
