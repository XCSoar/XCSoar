// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "system/Args.hpp"
#include "io/DataHandler.hpp"
#include "event/Loop.hxx"
#include "event/net/cares/Channel.hxx"
#include "util/PrintException.hxx"
#include "HexDump.hpp"

#include <chrono>

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

class SpliceHandler : public DataHandler {
  const unsigned no;

  Port *to_port = nullptr;

public:
  explicit SpliceHandler(unsigned _no) noexcept :no(_no) {}

  void SetToPort(Port *_to_port) noexcept {
    to_port = _to_port;
  }

  bool DataReceived(std::span<const std::byte> s) noexcept override {
    char prefix[32];
    sprintf(prefix, "[%u] %12llu ", no, (unsigned long long)
            std::chrono::steady_clock::now().time_since_epoch().count());
    HexDump(prefix, s);

    if (to_port != nullptr)
      to_port->Write(s);

    return true;
  }
};

int main(int argc, char **argv)
try {
  Args args{argc, argv, "PORT1 BAUD1 PORT2 BAUD2"};
  DebugPort debug_port1{args}, debug_port2{args};
  args.ExpectEnd();

  EventLoop event_loop;
  Cares::Channel cares{event_loop};

  SpliceHandler handler1{1}, handler2{2};

  auto port1 = debug_port1.Open(event_loop, cares, handler1);
  auto port2 = debug_port2.Open(event_loop, cares, handler2);

  handler1.SetToPort(port2.get());
  handler2.SetToPort(port1.get());

  MyListener listener1{event_loop, *port1}, listener2{event_loop, *port2};
  debug_port1.SetListener(listener1);
  debug_port2.SetListener(listener2);

  if (!port1->StartRxThread() || !port2->StartRxThread()) {
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  event_loop.Run();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
