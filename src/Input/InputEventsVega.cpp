// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "Dialogs/Device/Vega/VegaDialogs.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

static VegaDevice *
GetVegaDevice(DeviceDescriptor &device)
{
  return !device.IsOccupied() && device.GetState() == PortState::READY &&
    device.IsVega() && device.GetDevice() != nullptr
    ? (VegaDevice *)device.GetDevice()
    : NULL;
}

static void
AllVegasSendSetting(const char *name, int value)
{
  PopupOperationEnvironment env;

  for (DeviceDescriptor *i : *backend_components->devices) {
    VegaDevice *vega = GetVegaDevice(*i);
    if (vega != NULL)
      vega->SendSetting(name, value, env);
  }
}

static void
AllVegasRequestSetting(const char *name)
{
  PopupOperationEnvironment env;

  for (DeviceDescriptor *i : *backend_components->devices) {
    VegaDevice *vega = GetVegaDevice(*i);
    if (vega != NULL)
      vega->RequestSetting(name, env);
  }
}

// AdjustVarioFilter
// When connected to the Vega variometer, this adjusts
// the filter time constant
//     slow/medium/fast
// The following arguments can be used for diagnostics purposes
//     statistics:
//     diagnostics:
//     psraw:
//     switch:
// The following arguments can be used to trigger demo modes:
//     climbdemo:
//     stfdemo:
// Other arguments control vario setup:
//     save: saves the vario configuration to nonvolatile memory on the instrument
//     zero: Zero's the airspeed indicator's offset
//
void
InputEvents::eventAdjustVarioFilter(const char *misc)
{
  static int naccel = 0;
  if (StringIsEqual(misc, "slow"))
    AllVegasSendSetting("VarioTimeConstant", 3);
  else if (StringIsEqual(misc, "medium"))
    AllVegasSendSetting("VarioTimeConstant", 2);
  else if (StringIsEqual(misc, "fast"))
    AllVegasSendSetting("VarioTimeConstant", 1);
  else if (StringIsEqual(misc, "statistics"))
    AllVegasSendSetting("Diagnostics", 1);
  else if (StringIsEqual(misc, "diagnostics"))
    AllVegasSendSetting("Diagnostics", 2);
  else if (StringIsEqual(misc, "psraw"))
    AllVegasSendSetting("Diagnostics", 3);
  else if (StringIsEqual(misc, "switch"))
    AllVegasSendSetting("Diagnostics", 4);
  else if (StringIsEqual(misc, "democlimb")) {
    AllVegasSendSetting("DemoMode", 0);
    AllVegasSendSetting("DemoMode", 2);
  } else if (StringIsEqual(misc, "demostf")) {
    AllVegasSendSetting("DemoMode", 0);
    AllVegasSendSetting("DemoMode", 1);
  } else if (StringIsEqual(misc, "accel")) {
    switch (naccel) {
    case 0:
      AllVegasRequestSetting("AccelerometerSlopeX");
      break;
    case 1:
      AllVegasRequestSetting("AccelerometerSlopeY");
      break;
    case 2:
      AllVegasRequestSetting("AccelerometerOffsetX");
      break;
    case 3:
      AllVegasRequestSetting("AccelerometerOffsetY");
      break;
    default:
      naccel = 0;
      break;
    }
    naccel++;
    if (naccel > 3)
      naccel = 0;

  } else if (StringIsEqual(misc, "xdemo")) {
    dlgVegaDemoShowModal();
  } else if (StringIsEqual(misc, "zero")) {
    // zero, no mixing
    if (!CommonInterface::Calculated().flight.flying) {
      AllVegasSendSetting("ZeroASI", 1);
    }
  } else if (StringIsEqual(misc, "save")) {
    AllVegasSendSetting("StoreToEeprom", 2);

  // accel calibration
  } else if (!CommonInterface::Calculated().flight.flying) {
    if (StringIsEqual(misc, "X1"))
      AllVegasSendSetting("CalibrateAccel", 1);
    else if (StringIsEqual(misc, "X2"))
      AllVegasSendSetting("CalibrateAccel", 2);
    else if (StringIsEqual(misc, "X3"))
      AllVegasSendSetting("CalibrateAccel", 3);
    else if (StringIsEqual(misc, "X4"))
      AllVegasSendSetting("CalibrateAccel", 4);
    else if (StringIsEqual(misc, "X5"))
      AllVegasSendSetting("CalibrateAccel", 5);
  }
}
