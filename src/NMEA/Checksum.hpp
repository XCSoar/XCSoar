// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string_view>

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a NULL terminated string
 */
[[nodiscard]] [[gnu::pure]]
static constexpr uint8_t
NMEAChecksum(std::convertible_to<const char *> auto &&_src) noexcept
{
  const char *p = _src;

  uint8_t checksum = 0;

  /* skip the dollar sign at the beginning (the exclamation mark is
     used by CAI302 */
  if (*p == '$' || *p == '!')
    ++p;

  while (*p != 0)
    checksum ^= static_cast<uint8_t>(*p++);

  return checksum;
}

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a string
 * @param length the number of characters in the string
 */
[[nodiscard]] [[gnu::pure]]
static constexpr uint8_t
NMEAChecksum(std::string_view src) noexcept
{
  uint8_t checksum = 0;

  /* skip the dollar sign at the beginning (the exclamation mark is
     used by CAI302 */
  if (!src.empty() && (src.front() == '$' || src.front() == '!'))
    src.remove_prefix(1);

  for (char ch : src)
    checksum ^= static_cast<uint8_t>(ch);

  return checksum;
}

/**
 * Verify the NMEA checksum at the end of the specified string,
 * separated with an asterisk ('*').
 */
[[nodiscard]] [[gnu::pure]]
bool
VerifyNMEAChecksum(const char *p) noexcept;

/**
 * Caclulates the checksum of the specified string, and appends it at
 * the end, preceded by an asterisk ('*').
 */
void
AppendNMEAChecksum(char *p) noexcept;
