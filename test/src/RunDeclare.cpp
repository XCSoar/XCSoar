/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"
#include "Util/PrintException.hxx"

#define MORE_USAGE
#include "OS/Args.hpp"

#include <stdio.h>

void
PrintMoreUsage()
{
  fputs("Where DRIVER0 is one of:\n", stderr);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    if (driver->HasPassThrough())
      _ftprintf(stderr, _T("\t%s\n"), driver->name);

  fputs("Where DRIVER is one of:\n", stderr);

  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    if (driver->CanDeclare())
      _ftprintf(stderr, _T("\t%s\n"), driver->name);
}

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
NMEAParser::ReadTime(NMEAInputLine &line, BrokenTime &broken_time,
                     double &time_of_day_s)
{
  return false;
}

bool
NMEAParser::TimeHasAdvanced(double this_time, double &last_time, NMEAInfo &info)
{
  return false;
}

static Waypoint
MakeWaypoint(const TCHAR *name, int altitude,
             double longitude, double latitude)
{
  Waypoint wp(GeoPoint(Angle::Degrees(longitude),
                       Angle::Degrees(latitude)));
  wp.name = name;
  wp.elevation = altitude;
  return wp;
}

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

int main(int argc, char **argv)
try {
  Args args(argc, argv, "[--through DRIVER0] DRIVER PORT BAUD");

  tstring _through_name;
  const TCHAR *through_name = NULL;

  const char *a;
  while ((a = args.PeekNext()) != NULL && *a == '-') {
    a = args.ExpectNext();
    if (strcmp(a, "--through") == 0) {
      _through_name = args.ExpectNextT();
      through_name = _through_name.c_str();
    } else
      args.UsageError();
  }

  tstring _driver_name = args.ExpectNextT();
  const TCHAR *driver_name = _driver_name.c_str();
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *(DataHandler *)nullptr);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  LoggerSettings logger_settings;
  logger_settings.pilot_name = _T("Foo Bar");
  Plane plane;
  plane.registration = _T("D-3003");
  plane.competition_id = _T("33");
  plane.type = _T("Cirrus");

  Declaration declaration(logger_settings, plane, NULL);

  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));
  declaration.Append(MakeWaypoint(_T("Foo"), 488, 8, 52));
  declaration.Append(MakeWaypoint(_T("Bar"), 488, 7.5, 50));
  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));

  Device *through_device = NULL;
  if (through_name != NULL) {
    const struct DeviceRegister *through_driver =
      FindDriverByName(through_name);
    if (through_driver == NULL) {
      _ftprintf(stderr, _T("No such driver: %s\n"), through_name);
      return EXIT_FAILURE;
    }

    if (!through_driver->HasPassThrough()) {
      _ftprintf(stderr, _T("Not a pass-through driver: %s\n"), through_name);
      return EXIT_FAILURE;
    }

    assert(through_driver->CreateOnPort != NULL);
    through_device = through_driver->CreateOnPort(debug_port.GetConfig(),
                                                  *port);
    assert(through_device != NULL);
  }

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  if (!driver->CanDeclare()) {
    _ftprintf(stderr, _T("Not a logger driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(debug_port.GetConfig(), *port);
  assert(device != NULL);

  if (through_device != NULL && !through_device->EnablePassThrough(env)) {
    _ftprintf(stderr, _T("Failed to enable pass-through mode: %s\n"),
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
