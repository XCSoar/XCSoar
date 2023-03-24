// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "Device/Config.hpp"
#include "system/Args.hpp"
#include "system/Sleep.h"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "util/PrintException.hxx"

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
try {
  Args args(argc, argv, "DRIVER PORT BAUD");
  Emulator *emulator = LoadEmulator(args);
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *global_cares_channel,
                              *emulator->handler);

  emulator->port = port.get();

  ConsoleOperationEnvironment env;
  emulator->env = &env;

  if (!port->WaitConnected(env)) {
    delete emulator;
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  if (!port->StartRxThread()) {
    delete emulator;
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  while (port->GetState() != PortState::FAILED)
    Sleep(1000);

  delete emulator;
  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
