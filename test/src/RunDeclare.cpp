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

#include "Device/SerialPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Descriptor.hpp"
#include "Device/device.hpp"
#include "Device/Geoid.h"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "InputEvents.hpp"
#include "Thread/Trigger.hpp"
#include "DeviceBlackboard.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"

#include <stdio.h>

Waypoints way_points;

DeviceBlackboard device_blackboard;

static DeviceDescriptor device;

/*
 * Fake Protection.cpp
 */

Mutex mutexBlackboard;

void TriggerGPSUpdate() {}
void TriggerVarioUpdate() {}

/*
 * Fake Device/device.cpp
 */

bool
devHasBaroSource()
{
  return device.IsBaroSource();
}

bool
HaveCondorDevice()
{
  return device.IsCondor();
}

/*
 * Fake Device/Geoid.cpp
 */

fixed
LookupGeoidSeparation(const GeoPoint pt)
{
  return fixed_zero;
}

/*
 * Fake InputEvents.cpp
 */

bool
InputEvents::processGlideComputer(unsigned gce_id)
{
  return true;
}

bool
InputEvents::processNmea(unsigned key)
{
  return true;
}

/*
 * Fake Settings*Blackboard.cpp
 */

SettingsComputerBlackboard::SettingsComputerBlackboard() {}
SettingsMapBlackboard::SettingsMapBlackboard() {}

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
  wp.Name = name;
  wp.Altitude = fixed(altitude);
  return wp;
}

int main(int argc, char **argv)
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s DRIVER PORT BAUD\n"
            "Where DRIVER is one of:\n", argv[0]);

    const TCHAR *name;
    for (unsigned i = 0; (name = devRegisterGetName(i)) != NULL; ++i)
      _ftprintf(stderr, _T("\t%s\n"), name);

    return EXIT_FAILURE;
  }

  PathName driver_name(argv[1]);
  PathName port_name(argv[2]);
  int baud = atoi(argv[3]);

  device.Driver = devGetDriver(driver_name);
  if (device.Driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  SerialPort *port = new SerialPort(port_name, baud, *(Port::Handler *)NULL);
  device.Com = port;
  if (!port->Open()) {
    delete port;
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  if (!device.Open()) {
    delete device.Com;
    fprintf(stderr, "Failed to open driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  Declaration declaration(NULL);
  _tcscpy(declaration.PilotName, _T("Foo Bar"));
  _tcscpy(declaration.AircraftType, _T("Cirrus"));
  _tcscpy(declaration.AircraftRego, _T("D-3003"));

  declaration.append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));
  declaration.append(MakeWaypoint(_T("Foo"), 488, 8, 52));
  declaration.append(MakeWaypoint(_T("Bar"), 488, 7.5, 50));
  declaration.append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));

  if (device.Declare(&declaration))
    fprintf(stderr, "Declaration ok\n");
  else
    fprintf(stderr, "Declaration failed\n");

  device.Close();
}
