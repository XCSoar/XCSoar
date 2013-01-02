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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Profile/DeviceConfig.hpp"
#include "IO/Async/GlobalIOThread.hpp"

#define MORE_USAGE
#include "OS/Args.hpp"

#include <stdio.h>

void
PrintMoreUsage()
{
  fputs("Where DRIVER is one of:\n", stderr);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    _ftprintf(stderr, _T("\t%s\n"), driver->name);
}

bool
NMEAParser::ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r)
{
  return false;
}

bool
NMEAParser::ReadDate(NMEAInputLine &line, BrokenDate &date)
{
  return false;
}

bool
NMEAParser::TimeHasAdvanced(fixed this_time, fixed &last_time, NMEAInfo &info)
{
  return false;
}

fixed
NMEAParser::TimeModify(fixed fix_time, BrokenDateTime &date_time,
                       bool date_available)
{
  return fixed_zero;
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER PORT BAUD");

  tstring _driver_name = args.ExpectNextT();
  const TCHAR *driver_name = _driver_name.c_str();
  const DeviceConfig config = ParsePortArgs(args);
  args.ExpectEnd();

  InitialiseIOThread();

  Port *port = OpenPort(config, *(DataHandler *)NULL);
  if (port == NULL) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(config, *port);
  assert(device != NULL);

  ConsoleOperationEnvironment env;
  device->EnableNMEA(env);

  delete device;
  delete port;
  DeinitialiseIOThread();

  return EXIT_SUCCESS;
}
