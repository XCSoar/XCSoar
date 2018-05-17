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

#include "Device/Driver/CAI302/Internal.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "DebugPort.hpp"
#include "OS/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Util/Macros.hpp"
#include "Util/PrintException.hxx"
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"

#include <stdio.h>

gcc_pure
static int
StringBufferLength(const char *buffer, size_t size)
{
  const char *z = (const char *)memchr(buffer, 0, size);
  if (z == NULL)
    z = buffer + size;

  while (z > buffer && z[-1] == ' ')
    --z;

  return z - buffer;
}

static bool
PrintInfo(CAI302Device &device, OperationEnvironment &env)
{
  CAI302::GeneralInfo info;
  if (!device.ReadGeneralInfo(info, env))
    return false;

  printf("id: %.*s\n",
         StringBufferLength(info.id, ARRAY_SIZE(info.id)), info.id);
  printf("firmward: %.*s (%s)\n",
         StringBufferLength(info.version, ARRAY_SIZE(info.version)),
         info.version,
         info.type == 'F' ? "production"
         : (info.type == 'P' ? "prototype" : "unknown"));
  return true;
}

static bool
ListPilots(CAI302Device &device, OperationEnvironment &env)
{
  std::vector<CAI302::Pilot> pilots;
  unsigned active;
  if (!device.ReadPilotList(pilots, active, env))
    return false;

  unsigned index = 0;
  for (auto i = pilots.begin(), end = pilots.end(); i != end; ++i)
    printf("%u: '%.*s'\n", index++,
           StringBufferLength(i->name, ARRAY_SIZE(i->name)), i->name);

  printf("active=%u\n", active);

  return true;
}

static bool
ListNavpoints(CAI302Device &device, OperationEnvironment &env)
{
  const int count = device.ReadNavpointCount(env);
  if (count < 0)
    return false;

  printf("count=%u\n", count);
  for (int i = 0; i < count; ++i) {
    CAI302::Navpoint n;
    if (!device.ReadNavpoint(i, n, env))
      return false;

    int32_t latitude = FromBE32(n.latitude) - 54000000;
    char latitude_letter = latitude >= 0 ? 'N' : 'S';
    if (latitude < 0)
      latitude = -latitude;

    int32_t longitude = FromBE32(n.longitude) - 108000000;
    char longitude_letter = longitude >= 0 ? 'E' : 'W';
    if (longitude < 0)
      longitude = -longitude;

    printf("%u: %u:%02u:%02u%c %u:%02u:%02u%c '%.*s' '%.*s'\n", i,
           latitude / 10000 / 60,
           latitude / 10000 % 60,
           latitude * 6 / 1000 % 60,
           latitude_letter,
           longitude / 10000 / 60,
           longitude / 10000 % 60,
           longitude * 6 / 1000 % 60,
           longitude_letter,
           StringBufferLength(n.name, ARRAY_SIZE(n.name)), n.name,
           StringBufferLength(n.remark, ARRAY_SIZE(n.remark)), n.remark);
  }

  return true;
}

static bool
RunCommand(CAI302Device &device, const char *command,
           OperationEnvironment &env)
{
  if (strcmp(command, "info") == 0)
    return PrintInfo(device, env);
  else if (strcmp(command, "reboot") == 0)
    return device.Reboot(env);
  else if (strcmp(command, "poweroff") == 0)
    return device.PowerOff(env);
  else if (strcmp(command, "startlogger") == 0)
    return device.StartLogging(env);
  else if (strcmp(command, "stoplogger") == 0)
    return device.StopLogging(env);
  else if (strcmp(command, "clearlog") == 0)
    return device.ClearLog(env);
  else if (strcmp(command, "pilots") == 0)
    return ListPilots(device, env);
  else if (strcmp(command, "navpoints") == 0)
    return ListNavpoints(device, env);
  else {
    fprintf(stderr, "Unknown command: %s\n", command);
    return false;
  }
}

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

int main(int argc, char **argv)
try {
  const char *const usage = "PORT BAUD COMMAND\n\n"
    "Where COMMAND is one of:"
    "\n\tinfo"
    "\n\treboot"
    "\n\tpoweroff"
    "\n\tstartlogger"
    "\n\tstoplogger"
    "\n\tpilots"
    "\n\tnavpoints"
    ;
  Args args(argc, argv, usage);
  DebugPort debug_port(args);
  const char *command = args.ExpectNext();
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *(DataHandler *)nullptr);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  CAI302Device device(debug_port.GetConfig(), *port);
  if (!RunCommand(device, command, env)) {
    fprintf(stderr, "error\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
