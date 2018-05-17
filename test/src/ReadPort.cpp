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
#include "OS/Args.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"
#include "Util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD");
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *(DataHandler *)nullptr);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  char buffer[4096];
  while (true) {
    switch (port->WaitRead(env, 60000)) {
    case Port::WaitResult::READY:
      break;

    case Port::WaitResult::TIMEOUT:
      continue;

    case Port::WaitResult::FAILED:
      return EXIT_FAILURE;

    case Port::WaitResult::CANCELLED:
      return EXIT_SUCCESS;
    }

    int nbytes = port->Read(buffer, sizeof(buffer));
    if (nbytes < 0)
      break;

    fwrite((const void *)buffer, 1, nbytes, stdout);
  }

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
