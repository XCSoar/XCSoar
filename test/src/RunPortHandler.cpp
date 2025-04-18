// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "system/Args.hpp"
#include "io/DataHandler.hpp"
#include "event/Loop.hxx"
#include "event/net/cares/Channel.hxx"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

class MyListener final : public PortListener {
  EventLoop &event_loop;

  Port &port;

public:
  MyListener(EventLoop &_event_loop, Port &_port)
    :event_loop(_event_loop), port(_port) {}

  void PortStateChanged() noexcept override {
    if (port.GetState() == PortState::FAILED)
      event_loop.Break();
  }
};

class MyHandler : public DataHandler {
public:
  bool DataReceived(std::span<const std::byte> s) noexcept override {
    return fwrite(s.data(), 1, s.size(), stdout) == s.size();
  }
};

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD");
  DebugPort debug_port(args);
  args.ExpectEnd();

  EventLoop event_loop;
  Cares::Channel cares(event_loop);

  MyHandler handler;
  auto port = debug_port.Open(event_loop, cares, handler);
  MyListener listener(event_loop, *port);
  debug_port.SetListener(listener);

  ConsoleOperationEnvironment env;

  /* turn off output buffering */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (!port->StartRxThread()) {
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  event_loop.Run();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
