// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Config.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
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
  fputs("Where DRIVER is one of:\n", stderr);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
    _ftprintf(stderr, _T("\t%s\n"), driver->name);
}

bool
NMEAParser::ReadGeoPoint([[maybe_unused]] NMEAInputLine &line,
                         [[maybe_unused]] GeoPoint &value_r)
{
  return false;
}

bool
NMEAParser::ReadDate([[maybe_unused]] NMEAInputLine &line,
                     [[maybe_unused]] BrokenDate &date)
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

int main(int argc, char **argv)
try {
  Args args(argc, argv, "DRIVER PORT BAUD");

  tstring _driver_name = args.ExpectNextT();
  const char *driver_name = _driver_name.c_str();
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name);
    return EXIT_FAILURE;
  }

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(debug_port.GetConfig(), *port);
  assert(device != NULL);

  device->EnableNMEA(env);

  delete device;

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
