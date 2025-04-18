// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
