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

#include "Device/NullPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "InputEvents.hpp"
#include "DeviceBlackboard.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"
#include "Logger/IGCWriter.hpp"
#include "Profile/DeviceConfig.hpp"

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

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s DRIVER FILE.igc\n"
            "Where DRIVER is one of:\n", argv[0]);

    const TCHAR *name;
    for (unsigned i = 0; (name = GetDriverNameByIndex(i)) != NULL; ++i)
      _ftprintf(stderr, _T("\t%s\n"), name);

    return 1;
  }

  PathName driver_name(argv[1]);
  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return 1;
  }

  DeviceConfig config;
  config.Clear();

  NullPort port;
  Device *device = driver->CreateOnPort != NULL
    ? driver->CreateOnPort(config, &port)
    : NULL;

  NMEAParser parser;

  NMEAInfo data;
  data.Reset();

  char buffer[1024];
  for (unsigned i = 0; i < 10 &&
         fgets(buffer, sizeof(buffer), stdin) != NULL; ++i)
    if (device == NULL || !device->ParseNMEA(buffer, data))
      parser.ParseNMEAString_Internal(buffer, data);

  PathName igc_path(argv[2]);
  IGCWriter writer(igc_path, data);
  writer.header(data.date_time_utc,
                _T("Manfred Mustermann"), _T("Ventus"),
                _T("D-1234"), _T("Foo"), driver_name);

  GPSClock log_clock(fixed(1));
  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    TrimRight(buffer);

    if (device == NULL || !device->ParseNMEA(buffer, data))
      parser.ParseNMEAString_Internal(buffer, data);

    if (log_clock.check_advance(data.time))
      writer.LogPoint(data);
  }

  writer.flush();
}
