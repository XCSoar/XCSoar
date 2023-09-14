// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <cstddef>

extern const uint16_t crc16ccitt_table[256];

[[gnu::const]]
static inline uint16_t
UpdateCRC16CCITT(uint8_t octet, uint16_t crc)
{
  return (crc << 8) ^ crc16ccitt_table[(crc >> 8) ^ octet];
}

[[gnu::pure]]
static inline uint16_t
UpdateCRC16CCITT(const uint8_t *data, const uint8_t *end, uint16_t crc)
{
  while (data < end)
    crc = UpdateCRC16CCITT(*data++, crc);
  return crc;
}

[[gnu::pure]]
static inline uint16_t
UpdateCRC16CCITT(const void *data, size_t length, uint16_t crc)
{
  const uint8_t *p = (const uint8_t *)data, *end = p + length;
  return UpdateCRC16CCITT(p, end, crc);
}

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
