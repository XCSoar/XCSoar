// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Checksum.hpp"
#include "util/CRC16CCITT.hpp"

IMI::IMIWORD IMI::CRC16Checksum(const void *message, unsigned bytes)
{
  return UpdateCRC16CCITT(message, bytes, 0xffff);
}

IMI::IMIBYTE IMI::FixChecksum(const void *message, unsigned bytes)
{
  const IMIBYTE *p = (const IMIBYTE*)message;

  IMIBYTE checksum = 0;
  for (;bytes; bytes--)
    checksum ^= *p++;

  if (checksum == 0xFF) checksum = 0xAA;

  return checksum;
}
