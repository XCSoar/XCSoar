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

/*
 * This program creates a pseudo-TTY symlinked to /tmp/nmea, and feeds
 * NMEA data read from stdin to it.  It is useful to feed WINE with
 * it: symlink ~/.wine/dosdevices/com1 to /tmp/nmea, and configure
 * "COM1" in XCSoar.
 */

#include "FLARMEmulator.hpp"
#include "VegaEmulator.hpp"
#include "DebugPort.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Profile/DeviceConfig.hpp"
#include "OS/Args.hpp"
#include "OS/Sleep.h"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalIOThread.hpp"

#include <stdio.h>
#include <stdlib.h>

static Emulator *
LoadEmulator(Args &args)
{
  const char *driver = args.ExpectNext();
  if (strcmp(driver, "Vega") == 0)
    return new VegaEmulator();
  else if (strcmp(driver, "FLARM") == 0)
    return new FLARMEmulator();
  else {
    fprintf(stderr, "No such emulator driver: %s\n", driver);
    exit(EXIT_FAILURE);
  }
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER PORT BAUD");
  Emulator *emulator = LoadEmulator(args);
  const DeviceConfig config = ParsePortArgs(args);
  args.ExpectEnd();

  InitialiseIOThread();

  Port *port = OpenPort(config, *emulator->handler);
  if (port == NULL) {
    delete emulator;
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  /* turn off output buffering */
  setvbuf(stdout, NULL, _IONBF, 0);

  emulator->port = port;

  ConsoleOperationEnvironment env;
  emulator->env = &env;

  if (!port->StartRxThread()) {
    delete port;
    delete emulator;
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  while (port->IsValid())
    Sleep(1000);

  delete port;
  delete emulator;
  DeinitialiseIOThread();
  return EXIT_SUCCESS;
}
