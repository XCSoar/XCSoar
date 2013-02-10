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

#include "../Volkslogger.hpp"
#include "Internal.hpp"
#include "Profile/DeviceConfig.hpp"


static Device *
VolksloggerCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  unsigned bulkrate;
  //lowest value the Volkslogger api can accept as bulkrate is 9600
  if (config.bulk_baud_rate == 0)
    bulkrate = 115200; //Set fastest rate as default
  else if(config.bulk_baud_rate < 9600)
    bulkrate = 9600;
  else
    bulkrate = config.bulk_baud_rate;

  return new VolksloggerDevice(com_port, bulkrate);
}

const struct DeviceRegister volkslogger_driver = {
  _T("Volkslogger"),
  _T("Volkslogger"),
  DeviceRegister::DECLARE | DeviceRegister::LOGGER |
  DeviceRegister::BULK_BAUD_RATE,
  VolksloggerCreateOnPort,
};
