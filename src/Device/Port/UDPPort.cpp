// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UDPPort.hpp"
#include "net/IPv4Address.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "net/SocketError.hxx"

UDPPort::UDPPort(EventLoop &event_loop,
                 unsigned port,
                 PortListener *_listener, DataHandler &_handler)
  :SocketPort(event_loop, _listener, _handler)
{
  const IPv4Address address(port);

  UniqueSocketDescriptor s;
  if (!s.Create(AF_INET, SOCK_DGRAM, 0))
    throw MakeSocketError("Failed to create socket");

  if (!s.Bind(address))
    throw MakeSocketError("Failed to bind socket");

  OpenIndirect(s.Release());
}
