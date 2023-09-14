// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
SocketPort::Write(std::span<const std::byte> src)
{
  if (!socket.IsDefined())
    throw std::runtime_error("Port is closed");

  ssize_t nbytes = socket.GetSocket().Write(src.data(), src.size());
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
