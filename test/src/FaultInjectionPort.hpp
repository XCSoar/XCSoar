// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Port/Port.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include <stdio.h>

static unsigned inject_port_fault;

class FaultInjectionPort : public Port {
public:
  enum {
    DEFAULT_BAUD_RATE = 1234,
  };

  bool running;
  unsigned baud_rate;

  FaultInjectionPort(PortListener *_listener, DataHandler &_handler) noexcept
    :Port(_listener, _handler),
     running(true),
     baud_rate(DEFAULT_BAUD_RATE) {}

  /* virtual methods from class Port */
  PortState GetState() const noexcept override {
    return inject_port_fault > 0
      ? PortState::READY
      : PortState::FAILED;
  }

  std::size_t Write([[maybe_unused]] std::span<const std::byte> src) override {
    return src.size();
  }

  bool Drain() override {
    return true;
  }

  void Flush() override {}

  unsigned GetBaudrate() const noexcept override {
    return baud_rate;
  }

  void SetBaudrate(unsigned _baud_rate) override {
    baud_rate = _baud_rate;
  }

  bool StopRxThread() override {
    running = false;
    return true;
  }

  bool StartRxThread() override {
    running = true;
    return true;
  }

  std::size_t Read(std::span<std::byte> dest) override {
    if (inject_port_fault == 0)
      return 0;

    if (--inject_port_fault == 0)
      StateChanged();

    std::fill(dest.begin(), dest.end(), std::byte{' '});
    return dest.size();
  }

  void WaitRead([[maybe_unused]] std::chrono::steady_clock::duration timeout) override {
    if (inject_port_fault == 0)
      throw std::runtime_error{"Injected fault"};
  }
};
