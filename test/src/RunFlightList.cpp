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
#include "Device/Driver/FLARM/Device.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "OS/PathName.hpp"
#include "OS/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Profile/DeviceConfig.hpp"
#include "IO/Async/GlobalIOThread.hpp"

#include <stdio.h>

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

/*
 * The actual code.
 */

int main(int argc, char **argv)
{
  NarrowString<1024> usage;
  usage = "DRIVER PORT BAUD\n\n"
          "Where DRIVER is one of:";
  {
    const DeviceRegister *driver;
    for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i) {
      if (driver->IsLogger()) {
        NarrowPathName driver_name(driver->name);
        usage.AppendFormat("\n\t%s", (const char *)driver_name);
      }
    }
  }

  Args args(argc, argv, usage);
  tstring driver_name = args.ExpectNextT();
  const DeviceConfig config = ParsePortArgs(args);
  args.ExpectEnd();

  ConsoleOperationEnvironment env;
  RecordedFlightList flight_list;
  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (!driver->IsLogger()) {
    fprintf(stderr, "Not a logger driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);

  InitialiseIOThread();

  Port *port = OpenPort(config, *(DataHandler *)NULL);
  if (port == NULL) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  Device *device = driver->CreateOnPort(config, *port);
  assert(device != NULL);

  if (!device->ReadFlightList(flight_list, env)) {
    delete device;
    fprintf(stderr, "Failed to download flight list\n");
    return EXIT_FAILURE;
  }

  delete device;
  delete port;
  DeinitialiseIOThread();

  for (auto i = flight_list.begin(); i != flight_list.end(); ++i) {
    const RecordedFlightInfo &flight = *i;
    printf("%04u/%02u/%02u %02u:%02u-%02u:%02u\n",
           flight.date.year, flight.date.month, flight.date.day,
           flight.start_time.hour, flight.start_time.minute,
           flight.end_time.hour, flight.end_time.minute);
  }
}
