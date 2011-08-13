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

#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"
#include "Device/NullPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "InputEvents.hpp"
#include "Thread/Trigger.hpp"
#include "BasicComputer.hpp"
#include "Wind/WindZigZag.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Args.hpp"

#include <stdio.h>

const struct DeviceRegister *driver;

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
  Args args(argc, argv, "DRIVER");
  const tstring driver_name = args.ExpectNextT();

  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name.c_str());
    return 1;
  }

  args.ExpectEnd();

  DeviceConfig config;
  config.Clear();

  NullPort port;
  Device *device = driver->CreateOnPort != NULL
    ? driver->CreateOnPort(config, &port)
    : NULL;

  NMEAParser parser;

  MoreData data, last;
  data.Reset();
  data.UpdateClock();
  last = data;

  static DerivedInfo calculated;
  static SETTINGS_COMPUTER settings_computer;

  BasicComputer computer;

  printf("# time quality wind_bearing (deg) wind_speed (m/s) grndspeed (m/s) tas (m/s) bearing (deg)\n");
  char buffer[1024];

  WindZigZagGlue wind_zig_zag;

  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    TrimRight(buffer);

    if (data.time_available)
      data.clock = data.time;
    if (device == NULL || !device->ParseNMEA(buffer, data))
      parser.ParseNMEAString_Internal(buffer, data);

    if (data.location_available == last.location_available)
      continue;

    computer.Compute(data, last, calculated, settings_computer);
    calculated.flight.Moving(data.time);

    WindZigZagGlue::Result result = wind_zig_zag.Update(data, calculated);
    if (result.quality > 0)
      printf("%d %d %d %g %g %g %d\n", (int)data.time, result.quality,
             (int)result.wind.bearing.value_degrees(),
             (double)result.wind.norm,
             (double)data.ground_speed,
             (double)data.true_airspeed,
             (int)data.track.value_degrees());

    last = data;
  }
}

