/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "InputEvents.hpp"
#include "OS/PathName.hpp"
#include "ConsoleOperationEnvironment.hpp"
#include "Profile/DeviceConfig.hpp"

#ifdef HAVE_POSIX
#include "Device/TTYPort.hpp"
#else
#include "Device/SerialPort.hpp"
#endif

#include <stdio.h>

Waypoints way_points;

/*
 * Fake InputEvents.cpp
 */

bool
InputEvents::processNmea(unsigned key)
{
  return true;
}

/*
 * The actual code.
 */

static Waypoint
MakeWaypoint(const TCHAR *name, int altitude,
             double longitude, double latitude)
{
  const GeoPoint gp(Angle::degrees(fixed(7.7061111111111114)),
                    Angle::degrees(fixed(51.051944444444445)));
  Waypoint wp(GeoPoint(Angle::degrees(fixed(longitude)),
                       Angle::degrees(fixed(latitude))));
  wp.name = name;
  wp.altitude = fixed(altitude);
  return wp;
}

int main(int argc, char **argv)
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s DRIVER PORT BAUD\n"
            "Where DRIVER is one of:\n", argv[0]);

    const TCHAR *name;
    for (unsigned i = 0; (name = GetDriverNameByIndex(i)) != NULL; ++i)
      _ftprintf(stderr, _T("\t%s\n"), name);

    return EXIT_FAILURE;
  }

  PathName driver_name(argv[1]);
  PathName port_name(argv[2]);

  DeviceConfig config;
  config.Clear();
  config.baud_rate = atoi(argv[3]);

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (!driver->CanDeclare()) {
    fprintf(stderr, "Not a logger driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);

#ifdef HAVE_POSIX
  TTYPort *port = new TTYPort(port_name, config.baud_rate,
                              *(Port::Handler *)NULL);
#else
  SerialPort *port = new SerialPort(port_name, config.baud_rate,
                                    *(Port::Handler *)NULL);
#endif
  if (!port->Open()) {
    delete port;
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  Device *device = driver->CreateOnPort(config, port);
  assert(device != NULL);

  ConsoleOperationEnvironment env;
  if (!device->Open(env)) {
    delete port;
    fprintf(stderr, "Failed to open driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  Declaration declaration(NULL);
  declaration.pilot_name = _T("Foo Bar");
  declaration.aircraft_type = _T("Cirrus");
  declaration.aircraft_registration = _T("D-3003");
  declaration.competition_id = _T("33");

  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));
  declaration.Append(MakeWaypoint(_T("Foo"), 488, 8, 52));
  declaration.Append(MakeWaypoint(_T("Bar"), 488, 7.5, 50));
  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));

  if (device->Declare(declaration, env))
    fprintf(stderr, "Declaration ok\n");
  else
    fprintf(stderr, "Declaration failed\n");

  delete device;
  delete port;
}
