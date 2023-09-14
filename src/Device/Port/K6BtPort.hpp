// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Port.hpp"

#include <cstdint>
#include <memory>

/**
 * Wraps the K6Bt protocol over an existing Port instance.
 *
 * K6Bt is a Bluetooth to RS-232 adapter from K6-Team.
 */
class K6BtPort : public Port {
  static constexpr std::byte NOP{0x00};
  static constexpr std::byte ESCAPE{0xa5};
  static constexpr std::byte CHANGE_BAUD_RATE{0x30};
  static constexpr std::byte FLUSH_BUFFERS{0x40};

  std::unique_ptr<Port> port;

  unsigned baud_rate;

public:
  K6BtPort(std::unique_ptr<Port> port, unsigned baud_rate,
           PortListener *listener, DataHandler &handler) noexcept;

protected:
  bool SendCommand(std::byte cmd);
  void SendSetBaudrate(unsigned baud_rate);

public:
  /* virtual methods from Port */
  PortState GetState() const noexcept override;
  bool WaitConnected(OperationEnvironment &env) override;
  std::size_t Write(std::span<const std::byte> src) override;
  bool Drain() override;
  void Flush() override;
  void SetBaudrate(unsigned baud_rate) override;
  unsigned GetBaudrate() const noexcept override;
  bool StopRxThread() override;
  bool StartRxThread() override;
  std::size_t Read(std::span<std::byte> dest) override;
  void WaitRead(std::chrono::steady_clock::duration timeout) override;
};
