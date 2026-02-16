// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Device/Config.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/NullDataHandler.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>

bool
NMEAParser::ReadGeoPoint([[maybe_unused]] NMEAInputLine &line, [[maybe_unused]] GeoPoint &value_r)
{
  return false;
}

bool
NMEAParser::ReadDate([[maybe_unused]] NMEAInputLine &line, [[maybe_unused]] BrokenDate &date)
{
  return false;
}

bool
NMEAParser::ReadTime([[maybe_unused]] NMEAInputLine &line,
                     [[maybe_unused]] BrokenTime &broken_time,
                     [[maybe_unused]] TimeStamp &time_of_day_s) noexcept
{
  return false;
}

bool
NMEAParser::TimeHasAdvanced([[maybe_unused]] TimeStamp this_time,
                            [[maybe_unused]] TimeStamp &last_time,
                            [[maybe_unused]] NMEAInfo &info)
{
  return false;
}

/*
 * The actual code.
 */

int
main(int argc, char **argv)
try {
  StaticString<1024> usage;
  usage = "DRIVER PORT BAUD\n\n"
          "Where DRIVER is one of:";
  {
    const DeviceRegister *driver;
    for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i) {
      if (driver->IsLogger()) {
        usage.AppendFormat("\n\t%s", driver->name);
      }
    }
  }

  Args args(argc, argv, usage);
  std::string driver_name = args.ExpectNextT();
  DebugPort debug_port(args);
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

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  Device *device = driver->CreateOnPort(debug_port.GetConfig(), *port);
  assert(device != NULL);

  if (!device->ReadFlightList(flight_list, env)) {
    delete device;
    fprintf(stderr, "Failed to download flight list\n");
    return EXIT_FAILURE;
  }

  delete device;

  for (auto i = flight_list.begin(); i != flight_list.end(); ++i) {
    const RecordedFlightInfo &flight = *i;
    printf("%04u/%02u/%02u %02u:%02u-%02u:%02u\n",
           flight.date.year, flight.date.month, flight.date.day,
           flight.start_time.hour, flight.start_time.minute,
           flight.end_time.hour, flight.end_time.minute);
  }
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
