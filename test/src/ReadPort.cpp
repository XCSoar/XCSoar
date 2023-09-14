// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugPort.hpp"
#include "system/Args.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "Device/Error.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/NullDataHandler.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD");
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel,
                              handler);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  std::byte buffer[4096];
  while (true) {
    try {
      port->WaitRead(env, std::chrono::minutes(1));
    } catch (const DeviceTimeout &) {
      continue;
    }

    std::size_t nbytes = port->Read(std::span{buffer});
    fwrite((const void *)buffer, 1, nbytes, stdout);
  }

  return EXIT_SUCCESS;
} catch (OperationCancelled) {
  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
