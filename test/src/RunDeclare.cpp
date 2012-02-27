/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Device/Driver.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Logger/Settings.hpp"
#include "Plane/Plane.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Profile/DeviceConfig.hpp"

#define MORE_USAGE
#include "OS/Args.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>

void
PrintMoreUsage()
{
  fputs("Where DRIVER is one of:\n", stderr);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    if (driver->CanDeclare())
      _ftprintf(stderr, _T("\t%s\n"), driver->name);
}

bool
NMEAParser::ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r)
{
  return false;
}

static Waypoint
MakeWaypoint(const TCHAR *name, int altitude,
             double longitude, double latitude)
{
  Waypoint wp(GeoPoint(Angle::Degrees(fixed(longitude)),
                       Angle::Degrees(fixed(latitude))));
  wp.name = name;
  wp.altitude = fixed(altitude);
  return wp;
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER PORT BAUD");

  tstring _driver_name = args.ExpectNextT();
  const TCHAR *driver_name = _driver_name.c_str();
  tstring _port_name = args.ExpectNextT();
  const TCHAR *port_name = _port_name.c_str();
  const char *baud_string = args.ExpectNext();
  args.ExpectEnd();

  DeviceConfig config;
  config.Clear();
  config.baud_rate = atoi(baud_string);

#ifdef HAVE_POSIX
  TTYPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#else
  SerialPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#endif
  if (!port.Open()) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  LoggerSettings logger_settings;
  logger_settings.pilot_name = _T("Foo Bar");
  Plane plane;
  plane.registration = _T("D-3003");
  plane.competition_id = _T("33");
  plane.type = _T("Cirrus");

  Declaration declaration(logger_settings, plane, NULL);

  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));
  declaration.Append(MakeWaypoint(_T("Foo"), 488, 8, 52));
  declaration.Append(MakeWaypoint(_T("Bar"), 488, 7.5, 50));
  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  if (!driver->CanDeclare()) {
    _ftprintf(stderr, _T("Not a logger driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(config, port);
  assert(device != NULL);

  ConsoleOperationEnvironment env;
  if (!device->Open(env)) {
    _ftprintf(stderr, _T("Failed to open driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  if (device->Declare(declaration, NULL, env))
    fprintf(stderr, "Declaration ok\n");
  else
    fprintf(stderr, "Declaration failed\n");

  delete device;

  return EXIT_SUCCESS;
}
