// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

[[gnu::const]]
static inline const TCHAR *
GetStandardMonospaceFontFace() noexcept
{
  return _T("Consolas");
}

[[gnu::const]]
static inline const TCHAR *
GetStandardFontFace() noexcept
{
  return _T("Segeo UI");
}
