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

#include "Device/Driver/CAI302.hpp"
#include "Internal.hpp"

static Device *
CAI302CreateOnPort(const DeviceConfig &config, Port &port)
{
  return new CAI302Device(config, port);
}

const struct DeviceRegister cai302_driver = {
  _T("CAI 302"),
  _T("Cambridge CAI302"),
  DeviceRegister::BULK_BAUD_RATE |
  DeviceRegister::DECLARE | DeviceRegister::LOGGER | DeviceRegister::MANAGE |
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  CAI302CreateOnPort,
};
