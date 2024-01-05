// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Id.hpp"

#include <fmt/core.h>

#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <stdlib.h>

FlarmId
FlarmId::Parse(const char *input, char **endptr_r) noexcept
{
  return FlarmId(strtol(input, endptr_r, 16));
}

#ifdef _UNICODE
FlarmId
FlarmId::Parse(const TCHAR *input, TCHAR **endptr_r) noexcept
{
  return FlarmId(_tcstol(input, endptr_r, 16));
}
#endif

const char *
FlarmId::Format(char *buffer) const noexcept
{
  *fmt::format_to(buffer, "{:X}", value) = 0;
  return buffer;
}

#ifdef _UNICODE
const TCHAR *
FlarmId::Format(TCHAR *buffer) const noexcept
{
  *fmt::format_to(buffer, _T("{:X}"), value) = 0;
  return buffer;
}
#endif
