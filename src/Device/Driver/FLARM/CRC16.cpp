// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CRC16.hpp"
#include "util/CRC16CCITT.hpp"

#include <cassert>

uint16_t
FLARM::CalculateCRC(const FrameHeader &header,
                    const void *data, size_t length)
{
  assert((data != nullptr && length > 0) ||
         (data == nullptr && length == 0));

  uint16_t crc = 0x00;

  crc = UpdateCRC16CCITT((const uint8_t *)&header, 6, crc);

  if (length == 0 || data == nullptr)
    return crc;

  crc = UpdateCRC16CCITT((const uint8_t *)data, length, crc);

  return crc;
}
