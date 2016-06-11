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
#include "Device/Config.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"

DeviceConfig
ParsePortArgs(Args &args)
{
  DeviceConfig config;
  config.Clear();

  config.port_type = DeviceConfig::PortType::SERIAL;
  config.path = args.ExpectNextT().c_str();

#ifndef NDEBUG
  if (config.path.equals(_T("dump"))) {
    config = ParsePortArgs(args);
    config.dump_port = true;
    return config;
  }
#endif

  if (config.path.equals(_T("k6bt"))) {
    config = ParsePortArgs(args);
    config.k6bt = true;
    return config;
  }

  if (config.path.equals(_T("pty"))) {
    config.port_type = DeviceConfig::PortType::PTY;
    config.path = args.ExpectNextT().c_str();
    return config;
  }

  if (config.path.equals(_T("tcp"))) {
    config.port_type = DeviceConfig::PortType::TCP_LISTENER;
    config.tcp_port = atoi(args.ExpectNext());
    return config;
  }

  if (config.path.equals(_T("tcp_client"))) {
    config.port_type = DeviceConfig::PortType::TCP_CLIENT;
    config.ip_address = args.ExpectNextT().c_str();
    config.tcp_port = atoi(args.ExpectNext());
    return config;
  }

  if (config.path.equals(_T("udp"))) {
    config.port_type = DeviceConfig::PortType::UDP_LISTENER;
    config.tcp_port = atoi(args.ExpectNext());
    return config;
  }

  if (config.UsesSpeed()) {
    char *endptr;
    config.baud_rate = strtoul(args.ExpectNext(), &endptr, 10);

    if (*endptr == ':')
      config.bulk_baud_rate = atoi(endptr + 1);
  }

  return config;
}

std::unique_ptr<Port>
DebugPort::Open(boost::asio::io_service &io_service,
                DataHandler &handler)
{
  Port *port = OpenPort(io_service, config, this, handler);
  if (port == nullptr)
    throw std::runtime_error("Failed to open port");

  return std::unique_ptr<Port>(port);
}

void
DebugPort::PortStateChanged()
{
  if (listener != nullptr)
    listener->PortStateChanged();
}

void
DebugPort::PortError(const char *msg)
{
  fprintf(stderr, "Port error: %s\n", msg);

  if (listener != nullptr)
    listener->PortError(msg);
}
