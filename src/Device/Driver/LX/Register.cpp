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

#include "Device/Driver/LX.hpp"
#include "Internal.hpp"
#include "Device/Config.hpp"

static Device *
LXCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  const bool uses_speed = config.UsesSpeed();
  const unsigned baud_rate = uses_speed ? config.baud_rate : 0;
  const unsigned bulk_baud_rate = uses_speed ? config.bulk_baud_rate : 0;

  const bool is_nano = config.BluetoothNameStartsWith("LXNAV-NANO");

  return new LXDevice(com_port, baud_rate, bulk_baud_rate, is_nano);
}

const struct DeviceRegister lx_driver = {
  _T("LX"),
  _T("LXNAV"),
  DeviceRegister::DECLARE | DeviceRegister::LOGGER |
  DeviceRegister::PASS_THROUGH |
  DeviceRegister::BULK_BAUD_RATE |
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  LXCreateOnPort,
};
