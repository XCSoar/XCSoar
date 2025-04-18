// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/DataHandler.hpp"
#include "LineHandler.hpp"
#include "util/StaticFifoBuffer.hxx"

class PortLineSplitter : public DataHandler, protected PortLineHandler {
  typedef StaticFifoBuffer<char, 256u> Buffer;

  Buffer buffer;

public:
  /* virtual methods from class DataHandler */
  bool DataReceived(std::span<const std::byte> s) noexcept override;
};
