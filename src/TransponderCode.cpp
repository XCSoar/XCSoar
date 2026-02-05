// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TransponderCode.hpp"
#include "util/CharUtil.hxx"
#include "util/NumberParser.hpp"
#include "util/StringFormat.hpp"

char *
TransponderCode::Format(char *buffer, std::size_t max_size) const noexcept
{
  if (!IsDefined())
    return nullptr;

  StringFormat(buffer, max_size, _T("%04o"), value);
  return buffer;
}

TransponderCode
TransponderCode::Parse(const char *s) noexcept
{
  char *endptr;
  const auto value = ParseUnsigned(s, &endptr, 8);

  auto result = Null();
  if (endptr == s + 4 && IsWhitespaceOrNull(*endptr))
    result.value = value;

  return result;
}
