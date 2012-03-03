/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Profile/DeviceConfig.hpp"
#include "OS/Args.hpp"
#include "OS/PathName.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>

gcc_pure
static int
StringBufferLength(const char *buffer, size_t size)
{
  const char *z = (const char *)memchr(buffer, 0, size);
  if (z == NULL)
    return size;

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
  else {
    fprintf(stderr, "Unknown command: %s\n", command);
    return false;
  }
}

int main(int argc, char **argv)
{
  const char *const usage = "PORT BAUD COMMAND\n\n"
    "Where COMMAND is one of:"
    "\n\tinfo"
    "\n\treboot"
    "\n\tpoweroff"
    "\n\tstartlogger"
    "\n\tstoplogger"
    ;
  Args args(argc, argv, usage);

  PathName port_name(args.ExpectNext());

  DeviceConfig config;
  config.Clear();
  config.baud_rate = atoi(args.ExpectNext());

  const char *command = args.ExpectNext();
  args.ExpectEnd();

#ifdef HAVE_POSIX
  TTYPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#else
  SerialPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#endif
  if (!port.Open()) {
    fprintf(stderr, "Failed to open port\n");
    return EXIT_FAILURE;
  }

  ConsoleOperationEnvironment env;

  CAI302Device device(port);
  if (!device.Open(env)) {
    fprintf(stderr, "Failed to open driver\n");
    return EXIT_FAILURE;
  }

  if (!RunCommand(device, command, env)) {
    fprintf(stderr, "error\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
