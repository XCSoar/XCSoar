// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/State.hpp"
#include "LogFile.hpp"
#include "NativeInputListener.hpp"
#include "NativePortListener.hpp"
#include <cstddef>
#include <span>

class PortBridge {
public:
  PortBridge();

  void setListener(PortListener *listener);
  void setInputListener(DataHandler *handler);
  DataHandler *getInputListener();

  int getState() { return static_cast<int>(PortState::READY); }

  bool drain() { return true; }

  int getBaudRate() const { return -1; }

  bool setBaudRate(int baud_rate)
  {
    (void)baud_rate;
    return false;
  }

  virtual std::size_t write(std::span<const std::byte> src);

private:
  const PortListener *portListener;
  DataHandler *inputListener;
};

class iOSPortBridge : public PortBridge {
public:
  std::size_t write(std::span<const std::byte> src) override;
};
