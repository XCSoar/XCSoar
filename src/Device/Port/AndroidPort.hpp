// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BufferedPort.hpp"

class PortBridge;

/**
 * A #Port implementation which transmits data over a Bluetooth RFCOMM
 * socket.
 */
class AndroidPort : public BufferedPort
{
  PortBridge *bridge;

public:
  AndroidPort(PortListener *_listener, DataHandler &_handler,
              PortBridge *bridge);
  ~AndroidPort() noexcept override;

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;
  bool Drain() override;
  unsigned GetBaudrate() const noexcept override;
  void SetBaudrate(unsigned baud_rate) override;
  std::size_t Write(std::span<const std::byte> src) override;
};
