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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Device/Config.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/NullDataHandler.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD [NAME=VALUE] [NAME] ...");
  DebugPort debug_port(args);

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  VegaDevice device(*port);

  while (!args.IsEmpty()) {
    const char *p = args.GetNext();
    char *q = strdup(p);
    char *v = strchr(q, '=');
    if (v == NULL) {
      device.RequestSetting(q, env);
    } else {
      *v++ = 0;
      device.SendSetting(q, atoi(v), env);
    }

    free(q);
  }

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
