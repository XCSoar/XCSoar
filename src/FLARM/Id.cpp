// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Id.hpp"

#include <stdlib.h>
#include <stdio.h>

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
  sprintf(buffer, "%lX", (unsigned long)value);
  return buffer;
}

#ifdef _UNICODE
const TCHAR *
FlarmId::Format(TCHAR *buffer) const noexcept
{
  _stprintf(buffer, _T("%lX"), (unsigned long)value);
  return buffer;
}
#endif
