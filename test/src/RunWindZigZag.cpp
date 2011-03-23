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
#include "Device/Descriptor.hpp"
#include "Device/device.hpp"
#include "Device/Geoid.h"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "InputEvents.hpp"
#include "Thread/Trigger.hpp"
#include "DeviceBlackboard.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"
#include "Wind/WindZigZag.hpp"

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

void
DeviceBlackboard::tick()
{
  if (!Basic().acceleration.Available)
    SetBasic().acceleration.Gload = fixed_one;

  SetCalculated().flight.flying_state_moving(Basic().Time);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s DRIVER\n"
            "Where DRIVER is one of:\n", argv[0]);

    const TCHAR *name;
    for (unsigned i = 0; (name = devRegisterGetName(i)) != NULL; ++i)
      _ftprintf(stderr, _T("\t%s\n"), name);

    return 1;
  }

  PathName driver_name(argv[1]);
  const struct DeviceRegister *driver = devGetDriver(driver_name);
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return 1;
  }

  NullPort port;

  if (!device.Open(&port, driver)) {
    fprintf(stderr, "Failed to open driver: %s\n", argv[1]);
    return 1;
  }

  device.enable_baro = true;

  printf("# time quality wind_bearing (deg) wind_speed (m/s) grndspeed (m/s) tas (m/s) bearing (deg)\n");
  char buffer[1024];

  fixed speed(0);
  Angle bearing = Angle::native(fixed(0));

  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    device.LineReceived(buffer);

    device_blackboard.tick();

    int quality = WindZigZagUpdate(device_blackboard.Basic(),
                                   device_blackboard.Calculated(),
                                   speed, bearing);
    if (quality > 0)
      printf("%d %d %d %g %g %g %d\n", (int)device_blackboard.Basic().Time, quality,
             (int)bearing.value_degrees(),
             (double)speed,
             (double)device_blackboard.Basic().GroundSpeed,
             (double)device_blackboard.Basic().TrueAirspeed,
             (int)device_blackboard.Basic().TrackBearing.value_degrees());
  }
}

