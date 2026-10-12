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

  /** Set for BLE serial.  Classic Bluetooth and USB serial leave it
      false. */
  const bool ble_serial;

public:
  AndroidPort(PortListener *_listener, DataHandler &_handler,
              PortBridge *bridge, bool ble_serial = false);
  ~AndroidPort() noexcept override;

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;
  bool Drain() override;
  unsigned GetBaudrate() const noexcept override;
  void SetBaudrate(unsigned baud_rate) override;
  bool SetConnectionPriority(ConnectionPriority priority) noexcept override;
  bool RequestMtu(unsigned mtu) noexcept override;
  bool IsBleSerial() const noexcept override;
  std::size_t Write(std::span<const std::byte> src) override;
};
