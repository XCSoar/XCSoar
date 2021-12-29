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

#include "TCPClientPort.hpp"
#include "event/Call.hxx"

TCPClientPort::TCPClientPort(EventLoop &event_loop, Cares::Channel &cares,
                             const char *host, unsigned port,
                             PortListener *_listener, DataHandler &_handler)
  :SocketPort(event_loop, _listener, _handler)
{
  BlockingCall(GetEventLoop(), [this, &cares, host, port](){
    Cares::SimpleHandler &resolver_handler = *this;
    resolver.emplace(resolver_handler, port);
    resolver->Start(cares, host);
  });
}

TCPClientPort::~TCPClientPort() noexcept
{
  BlockingCall(GetEventLoop(), [this](){
    connect.reset();
    resolver.reset();
  });
}

void
TCPClientPort::OnResolverSuccess(std::forward_list<AllocatedSocketAddress> addresses) noexcept
{
  assert(resolver);
  resolver.reset();

  if (addresses.empty()) {
    state = PortState::FAILED;
    StateChanged();
    Error("No address");
    return;
  }

  ConnectSocketHandler &handler = *this;
  connect.emplace(GetEventLoop(), handler);
  connect->Connect(addresses.front(), std::chrono::seconds(30));
}

void
TCPClientPort::OnResolverError(std::exception_ptr error) noexcept
{
  assert(resolver);
  resolver.reset();

  state = PortState::FAILED;
  StateChanged();
  Error(error);
}

void
TCPClientPort::OnSocketConnectSuccess(UniqueSocketDescriptor &&fd) noexcept
{
  assert(connect);

#ifdef _WIN32
  const DWORD value = 1000;
#else
  const struct timeval value{1, 0};
#endif

  fd.SetOption(SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value));

  SocketPort::Open(fd.Release());

  connect.reset();

  state = PortState::READY;
  StateChanged();
}

void
TCPClientPort::OnSocketConnectError(std::exception_ptr ep) noexcept
{
  assert(connect);
  connect.reset();

  state = PortState::FAILED;
  StateChanged();
  Error(std::move(ep));
}

void
TCPClientPort::OnConnectionClosed()
{
  throw std::runtime_error("Connection closed by peer");
}

void
TCPClientPort::OnConnectionError() noexcept
{
  state = PortState::FAILED;
}
