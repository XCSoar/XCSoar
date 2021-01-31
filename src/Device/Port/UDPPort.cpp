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

#include "UDPPort.hpp"
#include "net/IPv4Address.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "net/SocketError.hxx"
#include "event/Call.hxx"

UDPPort::UDPPort(EventLoop &event_loop,
                 unsigned port,
                 PortListener *_listener, DataHandler &_handler)
  :BufferedPort(_listener, _handler),
   socket(event_loop, BIND_THIS_METHOD(OnSocketReady))
{
  const IPv4Address address(port);

  UniqueSocketDescriptor s;
  if (!s.Create(AF_INET, SOCK_DGRAM, 0))
    throw MakeSocketError("Failed to create socket");

  if (!s.Bind(address))
    throw MakeSocketError("Failed to bind socket");

  socket.Open(s.Release());

  BlockingCall(event_loop, [this](){
    socket.ScheduleRead();
  });
}

UDPPort::~UDPPort()
{
  BufferedPort::BeginClose();

  BlockingCall(GetEventLoop(), [this](){
    socket.Close();
  });

  BufferedPort::EndClose();
}

PortState
UDPPort::GetState() const
{
  if (socket.IsDefined())
    return PortState::READY;
  else
    return PortState::FAILED;
}

size_t
UDPPort::Write(const void *data, size_t length)
{
  if (!socket.IsDefined())
    return 0;

  ssize_t nbytes = socket.GetSocket().Write(data, length);
  if (nbytes < 0)
    // TODO check EAGAIN?
    return 0;

  return nbytes;
}

void
UDPPort::OnSocketReady(unsigned) noexcept
try {
  char input[4096];
  ssize_t nbytes = socket.GetSocket().Read(input, sizeof(input));
  if (nbytes < 0)
    throw MakeSocketError("Failed to receive");

  if (nbytes == 0) {
    socket.Close();
    StateChanged();
    return;
  }

  DataReceived(input, nbytes);
} catch (...) {
  socket.Close();
  StateChanged();
  Error(std::current_exception());
}
