/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "SystemSettings.hpp"
#include "Asset.hpp"

void
SystemSettings::SetDefaults()
{
  for (unsigned i = 0; i < devices.size(); ++i)
    devices[i].Clear();

  if (IsAndroid() || IsApple()) {
    devices[0].port_type = DeviceConfig::PortType::INTERNAL;
  } else {
    devices[0].port_type = DeviceConfig::PortType::SERIAL;
#ifdef _WIN32
    devices[0].path = _T("COM1:");
#else
    devices[0].path = _T("/dev/tty0");
#endif
    devices[0].baud_rate = 4800;
    devices[0].driver_name = _T("Generic");
  }
}
