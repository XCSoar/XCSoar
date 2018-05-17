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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "OS/Args.hpp"
#include "OS/Sleep.h"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"
#include "IO/DataHandler.hpp"
#include "Util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

class MyHandler : public DataHandler {
public:
  virtual void DataReceived(const void *data, size_t length) {
    fwrite(data, 1, length, stdout);
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
  auto port = debug_port.Open(*asio_thread, handler);
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

  unsigned long last_stamp = -1;
  char line[1024];
  while (fgets(line, sizeof(line), stdin) != NULL) {
    char *endptr;
    unsigned long current_stamp = strtoul(line, &endptr, 10);
    if (endptr == line || *endptr != ' ' || endptr[1] != '<')
      continue;

    char *start = endptr + 2;
    char *end = strchr(start, '>');
    if (end == NULL)
      continue;

    *end++ = '\n';
    *end = 0;

    if (current_stamp > last_stamp) {
      unsigned long delta_t = std::min(current_stamp - last_stamp, 1000ul);
      Sleep(delta_t);
    }

    last_stamp = current_stamp;

    if (!port->FullWrite(start, end - start, env, 1000)) {
      fprintf(stderr, "Failed to write to port\n");
      return EXIT_FAILURE;

    }
  }

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
