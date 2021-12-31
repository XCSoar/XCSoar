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

#pragma once

#include "BufferedPort.hpp"
#include "event/SocketEvent.hxx"

/**
 * A base class for socket-based ports.
 */
class SocketPort : public BufferedPort
{
  SocketEvent socket;

public:
  SocketPort(EventLoop &event_loop,
             PortListener *_listener, DataHandler &_handler) noexcept;

  ~SocketPort() noexcept override;

  auto &GetEventLoop() const noexcept {
    return socket.GetEventLoop();
  }

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;

  bool Drain() override {
    /* writes are synchronous */
    return true;
  }

  void SetBaudrate(unsigned) override {
  }

  unsigned GetBaudrate() const noexcept override {
    return 0;
  }

  std::size_t Write(const void *data, std::size_t length) override;

protected:
  void Open(SocketDescriptor s) noexcept;
  void OpenIndirect(SocketDescriptor s) noexcept;

  void Close() noexcept {
    socket.Close();
  }

  bool IsConnected() const noexcept {
    return socket.IsDefined();
  }

  SocketDescriptor GetSocket() const noexcept {
    return socket.GetSocket();
  }

  /**
   * Called when the connection is closed by the peer.  Exceptions
   * thrown by this method are passed to PortListener::PortError().
   */
  virtual void OnConnectionClosed() {}

  /**
   * Called when the connection fails.
   */
  virtual void OnConnectionError() noexcept {}

private:
  void OnSocketReady(unsigned events) noexcept;
};
