// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TCPClientPort.hpp"
#include "event/Call.hxx"
#include "net/UniqueSocketDescriptor.hxx"

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
TCPClientPort::OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept
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
