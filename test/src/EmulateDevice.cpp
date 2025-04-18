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
#include "ATR833Emulator.hpp"
#include "DebugPort.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "system/Args.hpp"
#include "system/Sleep.h"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "util/PrintException.hxx"
#include "util/StringAPI.hxx"

#include <stdio.h>
#include <stdlib.h>

static std::unique_ptr<DeviceEmulator>
LoadEmulator(Args &args)
{
  const char *driver = args.ExpectNext();
  if (StringIsEqual(driver, "Vega"))
    return std::make_unique<VegaEmulator>();
  else if (StringIsEqual(driver, "FLARM"))
    return std::make_unique<FLARMEmulator>();
  else if (StringIsEqual(driver, "ATR833"))
    return std::make_unique<ATR833Emulator>();
  else {
    fprintf(stderr, "No such emulator driver: %s\n", driver);
    exit(EXIT_FAILURE);
  }
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "DRIVER PORT BAUD");
  auto emulator = LoadEmulator(args);
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *global_cares_channel,
                              *emulator->handler);

  emulator->port = port.get();

  ConsoleOperationEnvironment env;
  emulator->env = &env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  if (!port->StartRxThread()) {
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  while (port->GetState() != PortState::FAILED)
    Sleep(1000);

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
