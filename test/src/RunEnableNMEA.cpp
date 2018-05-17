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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Config.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"
#include "Util/PrintException.hxx"

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
NMEAParser::ReadTime(NMEAInputLine &line, BrokenTime &broken_time,
                     double &time_of_day_s)
{
  return false;
}

bool
NMEAParser::TimeHasAdvanced(double this_time, double &last_time,
                            NMEAInfo &info)
{
  return false;
}

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

int main(int argc, char **argv)
try {
  Args args(argc, argv, "DRIVER PORT BAUD");

  tstring _driver_name = args.ExpectNextT();
  const TCHAR *driver_name = _driver_name.c_str();
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *(DataHandler *)nullptr);

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(debug_port.GetConfig(), *port);
  assert(device != NULL);

  device->EnableNMEA(env);

  delete device;

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
