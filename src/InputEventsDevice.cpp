/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"
#include "Dialogs/DeviceListDialog.hpp"
#include "Dialogs/Vega.hpp"
#include "Device/device.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"

#include <assert.h>
#include <tchar.h>
#include <string.h>

// SendNMEA
//  Sends a user-defined NMEA string to an external instrument.
//   The string sent is prefixed with the start character '$'
//   and appended with the checksum e.g. '*40'.  The user needs only
//   to provide the text in between the '$' and '*'.
//
void
InputEvents::eventSendNMEA(const TCHAR *misc)
{
  if (misc)
    VarioWriteNMEA(misc);
}

void
InputEvents::eventSendNMEAPort1(const TCHAR *misc)
{
  const unsigned i = 0;

  if (misc != NULL && i < NUMDEV)
    DeviceList[i].WriteNMEA(misc);
}

void
InputEvents::eventSendNMEAPort2(const TCHAR *misc)
{
  const unsigned i = 1;

  if (misc != NULL && i < NUMDEV)
    DeviceList[i].WriteNMEA(misc);
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
  if (_tcscmp(misc, _T("slow")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,VarioTimeConstant,3"));
  else if (_tcscmp(misc, _T("medium")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,VarioTimeConstant,2"));
  else if (_tcscmp(misc, _T("fast")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,VarioTimeConstant,1"));
  else if (_tcscmp(misc, _T("statistics")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,1"));
  } else if (_tcscmp(misc, _T("diagnostics")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,2"));
  } else if (_tcscmp(misc, _T("psraw")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,3"));
  else if (_tcscmp(misc, _T("switch")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,4"));
  else if (_tcscmp(misc, _T("democlimb")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,0"));
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,2"));
  } else if (_tcscmp(misc, _T("demostf"))==0) {
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,0"));
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,1"));
  } else if (_tcscmp(misc, _T("accel")) == 0) {
    switch (naccel) {
    case 0:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerSlopeX"));
      break;
    case 1:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerSlopeY"));
      break;
    case 2:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerOffsetX"));
      break;
    case 3:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerOffsetY"));
      break;
    default:
      naccel = 0;
      break;
    }
    naccel++;
    if (naccel > 3)
      naccel = 0;

  } else if (_tcscmp(misc, _T("xdemo")) == 0) {
    dlgVegaDemoShowModal();
  } else if (_tcscmp(misc, _T("zero"))==0) {
    // zero, no mixing
    if (!CommonInterface::Calculated().flight.flying) {
      VarioWriteNMEA(_T("PDVSC,S,ZeroASI,1"));
    }
  } else if (_tcscmp(misc, _T("save")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,StoreToEeprom,2"));

  // accel calibration
  } else if (!CommonInterface::Calculated().flight.flying) {
    if (_tcscmp(misc, _T("X1"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,1"));
    else if (_tcscmp(misc, _T("X2"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,2"));
    else if (_tcscmp(misc, _T("X3"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,3"));
    else if (_tcscmp(misc, _T("X4"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,4"));
    else if (_tcscmp(misc, _T("X5"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,5"));
  }
}

void
InputEvents::eventDevice(const TCHAR *misc)
{
  assert(misc != NULL);

  if (_tcscmp(misc, _T("list")) == 0)
    ShowDeviceList(CommonInterface::main_window,
                   CommonInterface::main_window.GetLook().dialog,
                   CommonInterface::main_window.GetLook().terminal);
}
