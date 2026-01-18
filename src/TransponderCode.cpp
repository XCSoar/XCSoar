// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TransponderCode.hpp"

#include <cstdio>

std::string
TransponderCode::Format() const noexcept
{
  if (!IsDefined())
    return {};

  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "%04o", value);
  return buffer;
}

TransponderCode
TransponderCode::Parse(std::string_view s) noexcept
{
  auto result = Null();
  if (s.size() != 4)
    return result;

  uint_least16_t value = 0;
  for (const char c : s) {
    if (c < '0' || c > '7')
      return result;
    value = (value << 3) + static_cast<uint_least16_t>(c - '0');
  }

  result.value = value;
  return result;
}
