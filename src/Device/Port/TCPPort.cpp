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

#include "TCPPort.hpp"
#include "net/IPv4Address.hxx"
#include "net/SocketError.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "event/Call.hxx"

TCPPort::TCPPort(EventLoop &event_loop,
                 unsigned port,
                 PortListener *_listener, DataHandler &_handler)
  :BufferedPort(_listener, _handler),
   listener(event_loop, BIND_THIS_METHOD(OnListenerReady)),
   connection(event_loop, BIND_THIS_METHOD(OnConnectionReady))
{
  const IPv4Address address(port);

  UniqueSocketDescriptor s;
  if (!s.Create(AF_INET, SOCK_STREAM, 0))
    throw MakeSocketError("Failed to create socket");

  if (!s.Bind(address))
    throw MakeSocketError("Failed to bind socket");

  if (!s.Listen(1))
    throw MakeSocketError("Failed to listen on socket");

  listener.Open(s.Release());

  BlockingCall(event_loop, [this](){
    listener.ScheduleRead();
  });
}

TCPPort::~TCPPort()
{
  BufferedPort::BeginClose();

  BlockingCall(GetEventLoop(), [this](){
    connection.Close();
    listener.Close();
  });

  BufferedPort::EndClose();
}

PortState
TCPPort::GetState() const
{
  if (connection.IsDefined())
    return PortState::READY;
  else if (listener.IsDefined())
    return PortState::LIMBO;
  else
    return PortState::FAILED;
}

size_t
TCPPort::Write(const void *data, size_t length)
{
  if (!connection.IsDefined())
    return 0;

  ssize_t nbytes = connection.GetSocket().Write(data, length);
  if (nbytes < 0)
    // TODO check EAGAIN?
    return 0;

  return nbytes;
}

void
TCPPort::OnListenerReady(unsigned) noexcept
try {
  SocketDescriptor s = listener.GetSocket().Accept();
  if (!s.IsDefined())
    throw MakeSocketError("Failed to accept");

#ifdef _WIN32
  const DWORD value = 1000;
#else
  const struct timeval value{1, 0};
#endif

  s.SetOption(SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value));

  connection.Close();
  connection.Open(s);
  connection.ScheduleRead();
} catch (...) {
  listener.Close();
  StateChanged();
  Error(std::current_exception());
}

void
TCPPort::OnConnectionReady(unsigned) noexcept
try {
  char input[4096];
  ssize_t nbytes = connection.GetSocket().Read(input, sizeof(input));
  if (nbytes < 0)
    throw MakeSocketError("Failed to receive");

  if (nbytes == 0) {
    connection.Close();
    StateChanged();
    return;
  }

  DataReceived(input, nbytes);
} catch (...) {
  connection.Close();
  StateChanged();
  Error(std::current_exception());
}
