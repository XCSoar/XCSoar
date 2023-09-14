// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a NULL terminated string
 */
[[gnu::pure]]
static inline uint8_t
NMEAChecksum(const char *p)
{
  uint8_t checksum = 0;

  /* skip the dollar sign at the beginning (the exclamation mark is
     used by CAI302 */
  if (*p == '$' || *p == '!')
    ++p;

  while (*p != 0)
    checksum ^= *p++;

  return checksum;
}

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a string
 * @param length the number of characters in the string
 */
[[gnu::pure]]
static inline uint8_t
NMEAChecksum(const char *p, unsigned length)
{
  uint8_t checksum = 0;

  unsigned i = 0;

  /* skip the dollar sign at the beginning (the exclamation mark is
     used by CAI302 */
  if (length > 0 && (*p == '$' || *p == '!')) {
    ++i;
    ++p;
  }

  for (; i < length; ++i)
    checksum ^= *p++;

  return checksum;
}

/**
 * Verify the NMEA checksum at the end of the specified string,
 * separated with an asterisk ('*').
 */
[[gnu::pure]]
bool
VerifyNMEAChecksum(const char *p);

/**
 * Caclulates the checksum of the specified string, and appends it at
 * the end, preceded by an asterisk ('*').
 */
void
AppendNMEAChecksum(char *p);
