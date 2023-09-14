// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Checksum.hpp"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

bool
VerifyNMEAChecksum(const char *p)
{
  assert(p != NULL);

  const char *asterisk = strrchr(p, '*');
  if (asterisk == NULL)
    return false;

  const char *checksum_string = asterisk + 1;
  char *endptr;
  unsigned long ReadCheckSum2 = strtoul(checksum_string, &endptr, 16);
  if (endptr == checksum_string || *endptr != 0 || ReadCheckSum2 >= 0x100)
    return false;

  uint8_t ReadCheckSum = (unsigned char)ReadCheckSum2;
  uint8_t CalcCheckSum = NMEAChecksum(p, asterisk - p);

  return CalcCheckSum == ReadCheckSum;
}

void
AppendNMEAChecksum(char *p)
{
  assert(p != NULL);

  sprintf(p + strlen(p), "*%02X", NMEAChecksum(p));
}
