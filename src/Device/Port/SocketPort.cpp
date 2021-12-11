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

#include "SocketPort.hpp"
#include "net/IPv4Address.hxx"
#include "net/SocketError.hxx"
#include "event/Call.hxx"

SocketPort::SocketPort(EventLoop &event_loop,
                       PortListener *_listener, DataHandler &_handler) noexcept
  :BufferedPort(_listener, _handler),
   socket(event_loop, BIND_THIS_METHOD(OnSocketReady))
{
}

void
SocketPort::OpenIndirect(SocketDescriptor s) noexcept
{
  assert(s.IsDefined());
  assert(!socket.IsDefined());

  socket.Open(s);

  BlockingCall(GetEventLoop(), [this](){
    socket.ScheduleRead();
  });
}

void
SocketPort::Open(SocketDescriptor s) noexcept
{
  assert(s.IsDefined());
  assert(!socket.IsDefined());

  socket.Open(s);
  socket.ScheduleRead();
}

SocketPort::~SocketPort() noexcept
{
  BlockingCall(GetEventLoop(), [this](){
    socket.Close();
  });
}

PortState
SocketPort::GetState() const noexcept
{
  if (IsConnected())
    return PortState::READY;
  else
    return PortState::FAILED;
}

std::size_t
SocketPort::Write(const void *data, std::size_t length)
{
  if (!socket.IsDefined())
    throw std::runtime_error("Port is closed");

  ssize_t nbytes = socket.GetSocket().Write(data, length);
  if (nbytes < 0)
    // TODO check EAGAIN?
    throw MakeSocketError("Failed to send");

  return nbytes;
}

inline void
SocketPort::OnSocketReady(unsigned) noexcept
try {
  std::byte input[4096];
  ssize_t nbytes = socket.GetSocket().Read(input, sizeof(input));
  if (nbytes < 0)
    throw MakeSocketError("Failed to receive");

  if (nbytes == 0) {
    socket.Close();
    OnConnectionClosed();
    StateChanged();
    return;
  }

  DataReceived({input, std::size_t(nbytes)});
} catch (...) {
  socket.Close();
  OnConnectionError();
  StateChanged();
  Error(std::current_exception());
}
