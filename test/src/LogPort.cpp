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
#include "Profile/DeviceConfig.hpp"
#include "OS/Args.hpp"
#include "OS/Sleep.h"
#include "OS/Clock.hpp"
#include "IO/Async/GlobalIOThread.hpp"
#include "IO/DataHandler.hpp"
#include "HexDump.hpp"

#include <stdio.h>
#include <stdlib.h>

class MyHandler : public DataHandler {
public:
  virtual void DataReceived(const void *data, size_t length) {
    TCHAR prefix[16];
    _stprintf(prefix, _T("%12u "), MonotonicClockMS());
    HexDump(prefix, data, length);
  }
};

int main(int argc, char **argv)
{
  Args args(argc, argv, "PORT BAUD");
  const DeviceConfig config = ParsePortArgs(args);
  args.ExpectEnd();

  InitialiseIOThread();

  MyHandler handler;
  Port *port = OpenPort(config, handler);
  if (port == NULL) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  if (!port->StartRxThread()) {
    delete port;
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  while (true)
    Sleep(10000);

  delete port;
  DeinitialiseIOThread();
  return EXIT_SUCCESS;
}
