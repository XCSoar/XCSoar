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

  size_t Write([[maybe_unused]] const void *data, size_t length) override {
    return length;
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

  std::size_t Read(void *Buffer, size_t Size) override {
    if (inject_port_fault == 0)
      return 0;

    if (--inject_port_fault == 0)
      StateChanged();

    char *p = (char *)Buffer;
    std::fill_n(p, Size, ' ');
    return Size;
  }

  void WaitRead([[maybe_unused]] std::chrono::steady_clock::duration timeout) override {
    if (inject_port_fault == 0)
      throw std::runtime_error{"Injected fault"};
  }
};
