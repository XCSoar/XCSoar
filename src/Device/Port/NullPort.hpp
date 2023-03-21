// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/DataHandler.hpp"
#include "Port.hpp"

/**
 * Generic NullPort thread handler class
 */
class NullPort : public Port, private DataHandler  {
public:
  NullPort();
  NullPort(DataHandler  &_handler);

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;
  std::size_t Write(const void *data, std::size_t length) override;
  bool Drain() override;
  void Flush() override;
  unsigned GetBaudrate() const noexcept override;
  void SetBaudrate(unsigned baud_rate) override;
  bool StopRxThread() override;
  bool StartRxThread() override;
  std::size_t Read(void *Buffer, std::size_t Size) override;

  [[noreturn]]
  void WaitRead(std::chrono::steady_clock::duration timeout) override;

private:
  /* virtual methods from class DataHandler */
  bool DataReceived(std::span<const std::byte> s) noexcept override;
};
