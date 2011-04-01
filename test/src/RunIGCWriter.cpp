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
#include "Engine/Waypoint/Waypoints.hpp"
#include "InputEvents.hpp"
#include "Thread/Trigger.hpp"
#include "DeviceBlackboard.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"
#include "Logger/IGCWriter.hpp"

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

/*
 * The actual code.
 */

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s DRIVER FILE.igc\n"
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

  char buffer[1024];
  for (unsigned i = 0; i < 10 &&
         fgets(buffer, sizeof(buffer), stdin) != NULL; ++i)
    device.LineReceived(buffer);

  PathName igc_path(argv[2]);
  IGCWriter writer(igc_path, device_blackboard.Basic());
  writer.header(device_blackboard.Basic().DateTime,
                _T("Manfred Mustermann"), _T("Ventus"),
                _T("D-1234"), _T("Foo"), driver_name);

  GPSClock log_clock(fixed(1));
  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    device.LineReceived(buffer);

    if (log_clock.check_advance(device_blackboard.Basic().Time))
      writer.LogPoint(device_blackboard.Basic());
  }

  writer.flush();
}
