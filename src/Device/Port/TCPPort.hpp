// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SocketPort.hpp"
#include "event/SocketEvent.hxx"

/**
 * A TCP listener port class.
 */
class TCPPort final : public SocketPort
{
  SocketEvent listener;

public:
  /**
   * Creates a new TCPPort object, but does not open it yet.
   *
   * @param handler the callback object for input received on the
   * port
   */
  TCPPort(EventLoop &event_loop,
          unsigned port,
          PortListener *_listener, DataHandler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  ~TCPPort() noexcept override;

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;

protected:
  void AsyncAccept() noexcept {
    listener.ScheduleRead();
  }

  void OnListenerReady(unsigned events) noexcept;
};
