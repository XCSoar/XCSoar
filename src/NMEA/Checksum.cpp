// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Checksum.hpp"
#include "util/CharUtil.hxx"
#include "util/HexString.hpp"

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdint>

bool
VerifyNMEAChecksum(std::string_view sentence) noexcept
{
  const auto asterisk = sentence.rfind('*');
  if (asterisk == sentence.npos)
    return false;

  const auto field = sentence.substr(asterisk + 1);
  if (field.size() != 2 || !IsHexDigit(field[0]) || !IsHexDigit(field[1]))
    return false;

  const auto received = ParseHexString<1>(field)[0];
  return std::byte{NMEAChecksum(sentence.substr(0, asterisk))} == received;
}

void
AppendNMEAChecksum(char *p) noexcept
{
  assert(p != nullptr);

  const std::size_t length = strlen(p);

  sprintf(p + length, "*%02X", NMEAChecksum({p, length}));
}
