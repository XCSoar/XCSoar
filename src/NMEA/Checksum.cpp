// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Checksum.hpp"

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
#if defined(__APPLE__) && (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE)
  uint8_t CalcCheckSum = NMEAChecksum(p);
#else
  uint8_t CalcCheckSum = NMEAChecksum({p, asterisk});
#endif

  return CalcCheckSum == ReadCheckSum;
}

void
AppendNMEAChecksum(char *p) noexcept
{
  assert(p != nullptr);

  const std::size_t length = strlen(p);

#if defined(__APPLE__) && (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE)
  sprintf(p + length, "*%02X", NMEAChecksum(p));
#else
  sprintf(p + length, "*%02X", NMEAChecksum({p, length}));
#endif
}
