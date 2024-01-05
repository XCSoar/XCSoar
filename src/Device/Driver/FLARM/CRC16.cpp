// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CRC16.hpp"
#include "util/CRC16CCITT.hpp"

uint16_t
FLARM::CalculateCRC(const FrameHeader &header,
                    std::span<const std::byte> payload) noexcept
{
  uint16_t crc = 0x00;

  crc = UpdateCRC16CCITT((const uint8_t *)&header, 6, crc);
  crc = UpdateCRC16CCITT(payload, crc);

  return crc;
}
