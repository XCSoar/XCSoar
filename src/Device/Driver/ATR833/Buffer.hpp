// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "Device/Port/Port.hpp"

#include <cstdint>
#include <cstddef>

class ATRBuffer {
  uint8_t fill = 0;
  std::byte checksum{};
  std::byte data[32];

public:
  explicit constexpr ATRBuffer(std::byte msg_id) noexcept {
    using namespace ATR833;

    data[fill++] = STX;
    Put(SYNC);
    Put(msg_id);
  }

  constexpr void Put(std::byte byte) noexcept {
    using namespace ATR833;

    data[fill++] = byte;
    checksum ^= byte;
    if (byte == STX) {
      data[fill++] = byte;
      checksum ^= byte;
    }
  }

  void Send(Port &port, OperationEnvironment &env) {
    data[fill++] = checksum;
    port.FullWrite(std::span{data}.first(fill), env, std::chrono::seconds(2));
  }
};
