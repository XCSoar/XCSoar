// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TransponderMode.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"

static const TCHAR *const mode_strings[] = {
  _T(""),
  _T("OFF"),
  _T("SBY"),
  _T("GND"),
  _T("ON"),
  _T("ALT"),
  _T("IDENT"),
};

const TCHAR *
TransponderMode::ToString(Mode mode) noexcept
{
  unsigned i = (unsigned)mode;
  return i < ARRAY_SIZE(mode_strings)
    ? mode_strings[i]
    : _T("Unknown");
}
