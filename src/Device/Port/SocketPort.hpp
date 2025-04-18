// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  std::size_t Write(std::span<const std::byte> src) override;

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
