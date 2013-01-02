/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

  if (IsAltair()) {
    devices[0].port_type = DeviceConfig::PortType::SERIAL;
    devices[0].path = _T("COM3:");
    devices[0].baud_rate = 38400;
    devices[0].driver_name = _T("Altair RU");

    devices[1].port_type = DeviceConfig::PortType::SERIAL;
    devices[1].path = _T("COM1:");
    devices[1].baud_rate = 38400;
    devices[1].driver_name = _T("Vega");

    devices[2].port_type = DeviceConfig::PortType::SERIAL;
    devices[2].path = _T("COM2:");
    devices[2].baud_rate = 38400;
    devices[2].driver_name = _T("NmeaOut");
  } else if (IsAndroid()) {
    devices[0].port_type = DeviceConfig::PortType::INTERNAL;
  } else {
    devices[0].port_type = DeviceConfig::PortType::SERIAL;
#ifdef WIN32
    devices[0].path = _T("COM1:");
#else
    devices[0].path = _T("/dev/tty0");
#endif
    devices[0].baud_rate = 4800;
    devices[0].driver_name = _T("Generic");
  }

#ifdef HAVE_MODEL_TYPE
  model_type = ModelType::GENERIC;
#endif
}
