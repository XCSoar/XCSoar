// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

[[gnu::pure]]
static inline uint8_t
Calculate8bitCRC(const uint8_t* msg, const int len, uint8_t crc, const uint8_t poly)
{
  for (int byte = 0; byte < len; byte++) {
    uint8_t d = static_cast<uint8_t>(msg[byte]);
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
Calculate8bitCRC(const std::byte* msg, const int len, uint8_t crc, const uint8_t poly) {
  return Calculate8bitCRC(reinterpret_cast<const uint8_t*>(msg),len,crc,poly);
}
