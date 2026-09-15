// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Config.hpp"

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
