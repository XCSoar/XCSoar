// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Device/device.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Components.hpp"
#include "Operation/PopupOperationEnvironment.hpp"

#include <cassert>

// SendNMEA
//  Sends a user-defined NMEA string to an external instrument.
//   The string sent is prefixed with the start character '$'
//   and appended with the checksum e.g. '*40'.  The user needs only
//   to provide the text in between the '$' and '*'.
//
void
InputEvents::eventSendNMEA(const TCHAR *misc)
{
  if (misc != NULL) {
    PopupOperationEnvironment env;
    VarioWriteNMEA(misc, env);
  }
}

void
InputEvents::eventSendNMEAPort1(const TCHAR *misc)
{
  const unsigned i = 0;

  if (misc != NULL && i < NUMDEV) {
    PopupOperationEnvironment env;
    (*devices)[i].WriteNMEA(misc, env);
  }
}

void
InputEvents::eventSendNMEAPort2(const TCHAR *misc)
{
  const unsigned i = 1;

  if (misc != NULL && i < NUMDEV) {
    PopupOperationEnvironment env;
    (*devices)[i].WriteNMEA(misc, env);
  }
}

void
InputEvents::eventDevice(const TCHAR *misc)
{
  assert(misc != NULL);

  if (StringIsEqual(misc, _T("list")))
    ShowDeviceList();
}
