/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "InputEvents.hpp"
#include "Dialogs/Device/Vega/VegaDialogs.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Operation/PopupOperationEnvironment.hpp"

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

  for (DeviceDescriptor *i : *devices) {
    VegaDevice *vega = GetVegaDevice(*i);
    if (vega != NULL)
      vega->SendSetting(name, value, env);
  }
}

static void
AllVegasRequestSetting(const char *name)
{
  PopupOperationEnvironment env;

  for (DeviceDescriptor *i : *devices) {
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
InputEvents::eventAdjustVarioFilter(const TCHAR *misc)
{
  static int naccel = 0;
  if (StringIsEqual(misc, _T("slow")))
    AllVegasSendSetting("VarioTimeConstant", 3);
  else if (StringIsEqual(misc, _T("medium")))
    AllVegasSendSetting("VarioTimeConstant", 2);
  else if (StringIsEqual(misc, _T("fast")))
    AllVegasSendSetting("VarioTimeConstant", 1);
  else if (StringIsEqual(misc, _T("statistics")))
    AllVegasSendSetting("Diagnostics", 1);
  else if (StringIsEqual(misc, _T("diagnostics")))
    AllVegasSendSetting("Diagnostics", 2);
  else if (StringIsEqual(misc, _T("psraw")))
    AllVegasSendSetting("Diagnostics", 3);
  else if (StringIsEqual(misc, _T("switch")))
    AllVegasSendSetting("Diagnostics", 4);
  else if (StringIsEqual(misc, _T("democlimb"))) {
    AllVegasSendSetting("DemoMode", 0);
    AllVegasSendSetting("DemoMode", 2);
  } else if (StringIsEqual(misc, _T("demostf"))) {
    AllVegasSendSetting("DemoMode", 0);
    AllVegasSendSetting("DemoMode", 1);
  } else if (StringIsEqual(misc, _T("accel"))) {
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

  } else if (StringIsEqual(misc, _T("xdemo"))) {
    dlgVegaDemoShowModal();
  } else if (StringIsEqual(misc, _T("zero"))) {
    // zero, no mixing
    if (!CommonInterface::Calculated().flight.flying) {
      AllVegasSendSetting("ZeroASI", 1);
    }
  } else if (StringIsEqual(misc, _T("save"))) {
    AllVegasSendSetting("StoreToEeprom", 2);

  // accel calibration
  } else if (!CommonInterface::Calculated().flight.flying) {
    if (StringIsEqual(misc, _T("X1")))
      AllVegasSendSetting("CalibrateAccel", 1);
    else if (StringIsEqual(misc, _T("X2")))
      AllVegasSendSetting("CalibrateAccel", 2);
    else if (StringIsEqual(misc, _T("X3")))
      AllVegasSendSetting("CalibrateAccel", 3);
    else if (StringIsEqual(misc, _T("X4")))
      AllVegasSendSetting("CalibrateAccel", 4);
    else if (StringIsEqual(misc, _T("X5")))
      AllVegasSendSetting("CalibrateAccel", 5);
  }
}
