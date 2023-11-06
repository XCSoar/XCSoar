// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <span>

constexpr std::byte
UpdateCRC8(std::byte d, std::byte crc) noexcept
{
  constexpr std::byte poly{0x69};

  for (int count = 8; --count >= 0; d <<= 1) {
    std::byte tmp = crc ^ d;
    crc <<= 1;
    if ((tmp & std::byte{0x80}) != std::byte{})
      crc ^= poly;
  }

  return crc;
}

constexpr std::byte
UpdateCRC8(std::span<const std::byte> src, std::byte crc) noexcept
{
  for (std::byte d : src)
    crc = UpdateCRC8(d, crc);

  return crc;
}
