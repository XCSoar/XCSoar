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
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
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

class MyHandler : public DataHandler {
public:
  bool DataReceived(std::span<const std::byte> s) noexcept override {
    char prefix[16];
    sprintf(prefix, "%12llu ", (unsigned long long)
            std::chrono::steady_clock::now().time_since_epoch().count());
    HexDump(prefix, s.data(), s.size());
    return true;
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
