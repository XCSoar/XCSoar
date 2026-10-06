// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SocketPort.hpp"
#include "event/net/ConnectSocket.hxx"
#include "event/net/cares/SimpleResolver.hxx"

#include <forward_list>
#include <optional>

namespace Cares { class Channel; }

/**
 * A #Port implementation that connects to a TCP port.
 */
class TCPClientPort final
  : public SocketPort, Cares::SimpleHandler, ConnectSocketHandler
{
  std::optional<Cares::SimpleResolver> resolver;
  std::optional<ConnectSocket> connect;

  /**
   * The resolved addresses not tried yet.  A name like "localhost"
   * resolves to both "::1" and "127.0.0.1", and a server may listen
   * on only one of them.
   */
  std::forward_list<AllocatedSocketAddress> addresses;

  PortState state = PortState::LIMBO;

public:
  TCPClientPort(EventLoop &event_loop, Cares::Channel &cares,
                const char *host, unsigned port,
                PortListener *_listener, DataHandler &_handler);
  ~TCPClientPort() noexcept override;

  /* virtual methods from class Port */
  PortState GetState() const noexcept override {
    return state;
  }

private:
  /**
   * Connect to the first address in #addresses and remove it from
   * the list.
   */
  void ConnectNext() noexcept;

  /* virtual methods from Cares::SimpleHandler */
  void OnResolverSuccess(std::forward_list<AllocatedSocketAddress> _addresses) noexcept override;
  void OnResolverError(std::exception_ptr error) noexcept override;

  /* virtual methods from ConnectSocketHandler */
  void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept override;
  void OnSocketConnectError(std::exception_ptr ep) noexcept override;

protected:
  /* virtual methods from SocketPort */
  [[noreturn]]
  void OnConnectionClosed() override;

  void OnConnectionError() noexcept override;
};
