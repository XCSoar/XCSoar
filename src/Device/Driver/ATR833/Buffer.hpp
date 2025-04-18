// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "RadioFrequency.hpp"

#include <cstdint>
#include <cstddef>

class ATRBuffer {
  uint_least8_t fill = 0;
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

  constexpr void Put(RadioFrequency f) noexcept {
    Put(static_cast<std::byte>(f.GetKiloHertz() / 1000));
    Put(static_cast<std::byte>((f.GetKiloHertz() % 1000) / 5));
  }

  constexpr std::span<const std::byte> Finish() noexcept {
    data[fill++] = checksum;
    return std::span{data}.first(fill);
  }
};
