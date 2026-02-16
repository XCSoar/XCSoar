// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

#include <cassert>

// SendNMEA
//  Sends a user-defined NMEA string to an external instrument.
//   The string sent is prefixed with the start character '$'
//   and appended with the checksum e.g. '*40'.  The user needs only
//   to provide the text in between the '$' and '*'.
//
void
InputEvents::eventSendNMEA(const char *misc)
{
  if (misc != NULL && backend_components->devices != nullptr) {
    PopupOperationEnvironment env;
    backend_components->devices->VegaWriteNMEA(misc, env);
  }
}

void
InputEvents::eventSendNMEAPort1(const char *misc)
{
  const unsigned i = 0;

  if (misc != NULL && i < NUMDEV) {
    PopupOperationEnvironment env;
    (*backend_components->devices)[i].WriteNMEA(misc, env);
  }
}

void
InputEvents::eventSendNMEAPort2(const char *misc)
{
  const unsigned i = 1;

  if (misc != NULL && i < NUMDEV) {
    PopupOperationEnvironment env;
    (*backend_components->devices)[i].WriteNMEA(misc, env);
  }
}

void
InputEvents::eventDevice(const char *misc)
{
  assert(misc != NULL);

  if (StringIsEqual(misc, "list"))
    ShowDeviceList(*backend_components->device_blackboard,
                   backend_components->devices.get());
}
