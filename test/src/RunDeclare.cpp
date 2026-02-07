// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Logger/Settings.hpp"
#include "Plane/Plane.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Declaration.hpp"
#include "Device/Config.hpp"
#include "DebugPort.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/NullDataHandler.hpp"
#include "util/PrintException.hxx"

#define MORE_USAGE
#include "system/Args.hpp"

#include <stdio.h>

void
PrintMoreUsage()
{
  fputs("Where DRIVER0 is one of:\n", stderr);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    if (driver->HasPassThrough())
      fprintf(stderr, "\t%s\n", driver->name);

  fputs("Where DRIVER is one of:\n", stderr);

  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    if (driver->CanDeclare())
      fprintf(stderr, "\t%s\n", driver->name);
}

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

static Waypoint
MakeWaypoint(const char *name, int altitude,
             double longitude, double latitude)
{
  Waypoint wp(GeoPoint(Angle::Degrees(longitude),
                       Angle::Degrees(latitude)));
  wp.name = name;
  wp.elevation = altitude;
  wp.has_elevation = true;
  return wp;
}

int main(int argc, char **argv)
try {
  Args args(argc, argv, "[--through DRIVER0] DRIVER PORT BAUD");

  std::string _through_name;
  const char *through_name = NULL;

  const char *a;
  while ((a = args.PeekNext()) != NULL && *a == '-') {
    a = args.ExpectNext();
    if (strcmp(a, "--through") == 0) {
      _through_name = args.ExpectNextT();
      through_name = _through_name.c_str();
    } else
      args.UsageError();
  }

  std::string _driver_name = args.ExpectNextT();
  const char *driver_name = _driver_name.c_str();
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  LoggerSettings logger_settings;
  logger_settings.pilot_name = "Foo Bar";
  Plane plane;
  plane.registration = "D-3003";
  plane.competition_id = "33";
  plane.type = "Cirrus";

  Declaration declaration(logger_settings, plane, NULL);

  declaration.Append(MakeWaypoint("Bergneustadt", 488,
                                  7.7061111111111114, 51.051944444444445));
  declaration.Append(MakeWaypoint("Foo", 488, 8, 52));
  declaration.Append(MakeWaypoint("Bar", 488, 7.5, 50));
  declaration.Append(MakeWaypoint("Bergneustadt", 488,
                                  7.7061111111111114, 51.051944444444445));

  Device *through_device = NULL;
  if (through_name != NULL) {
    const struct DeviceRegister *through_driver =
      FindDriverByName(through_name);
    if (through_driver == NULL) {
      fprintf(stderr, "No such driver: %s\n", through_name);
      return EXIT_FAILURE;
    }

    if (!through_driver->HasPassThrough()) {
      fprintf(stderr, "Not a pass-through driver: %s\n", through_name);
      return EXIT_FAILURE;
    }

    assert(through_driver->CreateOnPort != NULL);
    through_device = through_driver->CreateOnPort(debug_port.GetConfig(),
                                                  *port);
    assert(through_device != NULL);
  }

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", driver_name);
    return EXIT_FAILURE;
  }

  if (!driver->CanDeclare()) {
    fprintf(stderr, "Not a logger driver: %s\n", driver_name);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(debug_port.GetConfig(), *port);
  assert(device != NULL);

  if (through_device != NULL && !through_device->EnablePassThrough(env)) {
    fprintf(stderr, "Failed to enable pass-through mode: %s\n",
              through_name);
    return EXIT_FAILURE;
  }

  if (device->Declare(declaration, NULL, env))
    fprintf(stderr, "Declaration ok\n");
  else
    fprintf(stderr, "Declaration failed\n");

  delete through_device;
  delete device;

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
