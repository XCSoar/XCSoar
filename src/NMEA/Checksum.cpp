// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Checksum.hpp"
#include "util/StringFormat.hpp"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

bool
VerifyNMEAChecksum(const char *p) noexcept
{
  assert(p != nullptr);

  const char *asterisk = strrchr(p, '*');
  if (asterisk == nullptr)
    return false;

  const char *checksum_string = asterisk + 1;
  char *endptr;
  unsigned long ReadCheckSum2 = strtoul(checksum_string, &endptr, 16);
  if (endptr == checksum_string || *endptr != 0 || ReadCheckSum2 >= 0x100)
    return false;

  uint8_t ReadCheckSum = (unsigned char)ReadCheckSum2;
  uint8_t CalcCheckSum = NMEAChecksum({p, asterisk});

  return CalcCheckSum == ReadCheckSum;
}

bool
AppendNMEAChecksum(char *p, size_t capacity) noexcept
{
  assert(p != nullptr);

  if (p == nullptr || capacity < 4)
    return false;

  const char *const terminator = (const char *)memchr(p, '\0', capacity);
  if (terminator == nullptr)
    return false;

  const std::size_t length = terminator - p;
  assert(length + 4 <= capacity);
  if (length + 4 > capacity)
    return false;

  const auto checksum = static_cast<unsigned>(NMEAChecksum({p, length}));
  const int written = StringFormat(p + length, 4, "*%02X", checksum);
  return written >= 0 && written < 4;
}
