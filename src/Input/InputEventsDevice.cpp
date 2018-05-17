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
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Device/device.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Components.hpp"
#include "Operation/PopupOperationEnvironment.hpp"

#include <assert.h>

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
