// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/RecordedFlight.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "DebugPort.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "system/ConvertPathName.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "system/Args.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/NullDataHandler.hpp"
#include "util/ConvertString.hpp"
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

static void
PrintFlightList(const RecordedFlightList &flight_list)
{
  for (auto i = flight_list.begin(); i != flight_list.end(); ++i) {
    const RecordedFlightInfo &flight = *i;
    printf("%04u/%02u/%02u %02u:%02u-%02u:%02u\n",
           flight.date.year, flight.date.month, flight.date.day,
           flight.start_time.hour, flight.start_time.minute,
           flight.end_time.hour, flight.end_time.minute);
  }
}

int main(int argc, char **argv)
try {
  NarrowString<1024> usage;
  usage = "DRIVER PORT BAUD FILE.igc [FLIGHT NR]\n\n"
          "Where DRIVER is one of:";
  {
    const DeviceRegister *driver;
    for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i) {
      if (driver->IsLogger()) {
        WideToUTF8Converter driver_name(driver->name);
        usage.AppendFormat("\n\t%s", (const char *)driver_name);
      }
    }
  }

  Args args(argc, argv, usage);
  std::string driver_name = args.ExpectNextT();
  DebugPort debug_port(args);

  const auto path = args.ExpectNextPath();

  unsigned flight_id = args.IsEmpty() ? 0 : atoi(args.GetNext());
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);

  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    args.UsageError();
  }

  if (!driver->IsLogger()) {
    fprintf(stderr, "Not a logger driver: %s\n", argv[1]);
    args.UsageError();
  }

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(debug_port.GetConfig(), *port);
  assert(device != NULL);

  RecordedFlightList flight_list;
  if (!device->ReadFlightList(flight_list, env)) {
    delete device;
    fprintf(stderr, "Failed to download flight list\n");
    return EXIT_FAILURE;
  }

  if (flight_list.empty()) {
    delete device;
    fprintf(stderr, "Logger is empty\n");
    return EXIT_FAILURE;
  }

  PrintFlightList(flight_list);

  if (flight_id >= flight_list.size()) {
    delete device;
    fprintf(stderr, "Flight id not found\n");
    return EXIT_FAILURE;
  }

  if (!device->DownloadFlight(flight_list[flight_id], path, env)) {
    delete device;
    fprintf(stderr, "Failed to download flight\n");
    return EXIT_FAILURE;
  }

  delete device;

  printf("Flight downloaded successfully\n");

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
