// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Vega.hpp"
#include "Internal.hpp"
#include "Operation/Operation.hpp"
#include "Blackboard/DeviceBlackboard.hpp"

// Forward declaration - avoid pulling in UI dependencies
class Device;
bool dlgConfigurationVarioShowModal(Device &device);

void
VegaDevice::LinkTimeout()
{
  AbstractDevice::LinkTimeout();
  detected = false;

  {
    const std::lock_guard<Mutex> lock(settings);
    settings.clear();
  }
}

void
VegaDevice::OnCalculatedUpdate([[maybe_unused]] const MoreData &basic,
                               const DerivedInfo &calculated)
{
  volatile_data.CopyFrom(calculated);

  if (detected) {
    NullOperationEnvironment env;
    volatile_data.SendTo(port, env);
  }

#ifdef UAV_APPLICATION
  const ThermalLocatorInfo &t = calculated.thermal_locator;
  char tbuf[100];
  sprintf(tbuf, "PTLOC,%d,%3.5f,%3.5f,%g,%g",
          (int)(t.estimate_valid),
          t.estimate_location.Longitude.Degrees(),
          t.estimate_location.Latitude.Degrees(),
          0.,
          0.);

  PortWriteNMEA(port, tbuf);
#endif
}

bool
VegaDevice::Manage([[maybe_unused]] unsigned device_index,
                   [[maybe_unused]] DeviceBlackboard &device_blackboard)
{
  dlgConfigurationVarioShowModal(*this);
  return true;
}
