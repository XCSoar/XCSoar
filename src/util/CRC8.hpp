// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <span>

#ifndef HAVE_POSIX
// on Windows the operators <<=, ^= and ^ not allowed yet for std::byte
constexpr uint8_t
UpdateCRC8(uint8_t d, uint8_t crc) noexcept
{
  constexpr uint8_t poly{0x69};

  for (int count = 8; --count >= 0; d <<= 1) {
    uint8_t tmp = crc ^ d;
    crc <<= 1;
    if ((tmp & 0x80) != 0)
      crc ^= poly;
  }
  return crc;
}
#endif

constexpr std::byte
UpdateCRC8(std::byte d, std::byte crc) noexcept
{
#ifdef HAVE_POSIX
  constexpr std::byte poly{0x69};

  for (int count = 8; --count >= 0; d <<= 1) {
    std::byte tmp = crc ^ d;
    crc <<= 1;
    if ((tmp & std::byte{0x80}) != std::byte{})
      crc ^= poly;
  }
  return crc;
#else
  return std::byte{UpdateCRC8((uint8_t)d, (uint8_t)crc)};
#endif
}

constexpr std::byte
UpdateCRC8(std::span<const std::byte> src, std::byte crc) noexcept
{
  for (std::byte d : src)
    crc = UpdateCRC8(d, crc);

  return crc;
}
