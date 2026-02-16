// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConsoleOperationEnvironment.hpp"

#include <stdio.h>

void
ConsoleOperationEnvironment::SetErrorMessage(const char *text) noexcept
{
  _ftprintf(stderr, _T("ERROR: %s\n"), text);
}

void
ConsoleOperationEnvironment::SetText(const char *text) noexcept
{
  _tprintf(_T("%s\n"), text);
}

void
ConsoleOperationEnvironment::SetProgressRange(unsigned _range) noexcept
{
  range = _range;
}

void
ConsoleOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  printf("%4u%%\n", position * 100 / range);
}
