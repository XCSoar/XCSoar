// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

extern const uint16_t crc16ccitt_table[256];

[[gnu::const]]
static inline uint16_t
UpdateCRC16CCITT(uint8_t octet, uint16_t crc) noexcept
{
  return (crc << 8) ^ crc16ccitt_table[(crc >> 8) ^ octet];
}

[[gnu::const]]
static inline uint16_t
UpdateCRC16CCITT(std::byte octet, uint16_t crc) noexcept
{
  return UpdateCRC16CCITT(static_cast<uint8_t>(octet), crc);
}

[[gnu::pure]]
static inline uint16_t
UpdateCRC16CCITT(const uint8_t *data, const uint8_t *end, uint16_t crc) noexcept
{
  while (data < end)
    crc = UpdateCRC16CCITT(*data++, crc);
  return crc;
}

[[gnu::pure]]
static inline uint16_t
UpdateCRC16CCITT(const void *data, size_t length, uint16_t crc) noexcept
{
  const uint8_t *p = (const uint8_t *)data, *end = p + length;
  return UpdateCRC16CCITT(p, end, crc);
}

[[gnu::pure]]
static inline uint16_t
UpdateCRC16CCITT(std::span<const std::byte> src, uint16_t crc)
{
  for (auto i : src)
    crc = UpdateCRC16CCITT(i, crc);
  return crc;
}
