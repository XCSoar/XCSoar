// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

class PortListener;
class DataHandler;

class PortBridge {
public:
  static void Initialise();

  PortBridge();

  void setListener(PortListener *listener);
  void setInputListener(DataHandler *handler);

  int getState()
  {
    // TODO
    return -1;
  }

  bool drain()
  {
    // TODO
    return false;
  }

  int getBaudRate() const
  {
    // TODO
    return -1;
  }

  bool setBaudRate(int baud_rate)
  {
    // TODO
    (void)baud_rate;
    return false;
  }

  std::size_t write(std::span<const std::byte> src);
};
