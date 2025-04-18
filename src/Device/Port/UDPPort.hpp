// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SocketPort.hpp"

/**
 * A UDP listener port class.
 */
class UDPPort final : public SocketPort
{
public:
  /**
   * Creates a new UDPPort object, but does not open it yet.
   *
   * @param handler the callback object for input received on the
   * port
   */
  UDPPort(EventLoop &event_loop,
          unsigned port,
          PortListener *_listener, DataHandler &_handler);
};
