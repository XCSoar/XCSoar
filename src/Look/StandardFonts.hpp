// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

[[gnu::const]]
static inline const char *
GetStandardMonospaceFontFace() noexcept
{
  return _T("Consolas");
}

[[gnu::const]]
static inline const char *
GetStandardFontFace() noexcept
{
  return _T("Segeo UI");
}
