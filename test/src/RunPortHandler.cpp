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
#include "OS/Args.hpp"
#include "IO/DataHandler.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Util/PrintException.hxx"

#include <boost/asio/io_service.hpp>

#include <stdio.h>
#include <stdlib.h>

class MyListener final : public PortListener {
  boost::asio::io_service &io_service;

  Port &port;

public:
  MyListener(boost::asio::io_service &_io_service, Port &_port)
    :io_service(_io_service), port(_port) {}

  void PortStateChanged() override {
    if (port.GetState() == PortState::FAILED)
      io_service.stop();
  }
};

class MyHandler : public DataHandler {
public:
  virtual void DataReceived(const void *data, size_t length) {
    fwrite(data, 1, length, stdout);
  }
};

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD");
  DebugPort debug_port(args);
  args.ExpectEnd();

  boost::asio::io_service io_service;

  MyHandler handler;
  auto port = debug_port.Open(io_service, handler);
  MyListener listener(io_service, *port);
  debug_port.SetListener(listener);

  ConsoleOperationEnvironment env;

  /* turn off output buffering */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (!port->StartRxThread()) {
    fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  io_service.run();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
