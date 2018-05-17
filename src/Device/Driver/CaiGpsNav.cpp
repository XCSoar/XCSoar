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

//


// CAUTION!
// caiGpsNavParseNMEA is called from com port read thread
// all other functions are called from windows message loop thread

#include "Device/Driver/CaiGpsNav.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"

#include <tchar.h>

static constexpr char CtrlC = '\x03';

class CaiGpsNavDevice : public AbstractDevice {
private:
  Port &port;

public:
  CaiGpsNavDevice(Port &_port):port(_port) {}

public:
  bool EnableNMEA(OperationEnvironment &env) override;
};

bool
CaiGpsNavDevice::EnableNMEA(OperationEnvironment &env)
{
  port.Write(CtrlC);
  env.Sleep(200);
  port.Write("NMEA\r");

  // This is for a slightly different mode, that
  // apparently outputs pressure info too...
  //port.Write("PNP\r\n");
  //port.Write("LOG 0\r\n");

  return true;
}

static Device *
CaiGpsNavCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new CaiGpsNavDevice(com_port);
}

const struct DeviceRegister gps_nav_driver = {
  _T("CAI GPS-NAV"),
  _T("Cambridge CAI GPS-NAV"),
  0,
  CaiGpsNavCreateOnPort,
};
