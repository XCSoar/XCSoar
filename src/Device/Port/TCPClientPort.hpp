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

#ifndef XCSOAR_DEVICE_TCP_CLIENT_PORT_HPP
#define XCSOAR_DEVICE_TCP_CLIENT_PORT_HPP

#include "BufferedPort.hpp"
#include "event/SocketEvent.hxx"
#include "event/net/ConnectSocket.hxx"
#include "event/net/cares/SimpleResolver.hxx"
#include "util/Cancellable.hxx"

#include <optional>

namespace Cares { class Channel; }

/**
 * A #Port implementation that connects to a TCP port.
 */
class TCPClientPort final
  : public BufferedPort, Cares::SimpleHandler, ConnectSocketHandler
{
  std::optional<Cares::SimpleResolver> resolver;
  std::optional<ConnectSocket> connect;
  SocketEvent socket;

  PortState state = PortState::LIMBO;

public:
  TCPClientPort(EventLoop &event_loop, Cares::Channel &cares,
                const char *host, unsigned port,
                PortListener *_listener, DataHandler &_handler);
  virtual ~TCPClientPort();

  auto &GetEventLoop() const noexcept {
    return socket.GetEventLoop();
  }

  /* virtual methods from class Port */
  PortState GetState() const override {
    return state;
  }

  bool Drain() override {
    /* writes are synchronous */
    return true;
  }

  bool SetBaudrate(unsigned baud_rate) override {
    return true;
  }

  unsigned GetBaudrate() const override {
    return 0;
  }

  size_t Write(const void *data, size_t length) override;

private:
  void OnSocketReady(unsigned events) noexcept;

  /* virtual methods from Cares::SimpleHandler */
  void OnResolverSuccess(std::forward_list<AllocatedSocketAddress> addresses) noexcept override;
  void OnResolverError(std::exception_ptr error) noexcept override;

  /* virtual methods from ConnectSocketHandler */
  void OnSocketConnectSuccess(UniqueSocketDescriptor &&fd) noexcept override;
  void OnSocketConnectError(std::exception_ptr ep) noexcept override;
};

#endif
