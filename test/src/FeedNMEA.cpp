/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "system/Args.hpp"
#include "system/Sleep.h"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/DataHandler.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

class MyHandler : public DataHandler {
public:
  bool DataReceived(std::span<const std::byte> s) noexcept override {
    return fwrite(s.data(), 1, s.size(), stdout) == s.size();
  }
};

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD");
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  MyHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);
  if (port == NULL) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  if (!port->StartRxThread()) {
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  char stamp[6] = "";

  char line[1024];
  while (fgets(line, sizeof(line), stdin) != NULL) {
    if (memcmp(line, "$GP", 3) == 0 &&
        (memcmp(line + 3, "GGA", 3) == 0 ||
         memcmp(line + 3, "RMC", 3) == 0) &&
        line[6] == ',' &&
        !StringIsEqual(stamp, line + 7, sizeof(stamp))) {
      /* the time stamp has changed - sleep for one second */
      Sleep(1000);
      strncpy(stamp, line + 7, sizeof(stamp));
    }

    port->FullWriteString(line, env, std::chrono::seconds(1));
  }

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
