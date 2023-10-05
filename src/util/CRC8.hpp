// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SpanCast.hxx"

#include <cstdint>
#include <span>

[[gnu::pure]]
static inline uint8_t
Calculate8bitCRC(std::span<const uint8_t> src, uint8_t crc, const uint8_t poly) noexcept
{
  for (uint8_t d : src) {
    for (int count = 8; --count >= 0; d <<= 1) {
      uint8_t tmp = crc ^ d;
      crc <<= 1;
      if ((tmp & 0x80) != 0)
        crc ^= poly;
    }
  }

  return crc;
}

[[gnu::pure]]
static inline uint8_t
Calculate8bitCRC(std::span<const std::byte> src, uint8_t crc, const uint8_t poly) noexcept
{
  return Calculate8bitCRC(FromBytesStrict<const uint8_t>(src), crc, poly);
}
