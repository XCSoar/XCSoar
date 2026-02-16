// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Date.hpp"
#include "Formatter/TimeFormatter.hpp"

const char *
DataFieldDate::GetAsString() const noexcept
{
  FormatISO8601(string_buffer, value);
  return string_buffer;
}
