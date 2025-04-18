// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Contests.hpp"
#include "util/Macros.hpp"

static const TCHAR *const contest_to_string[] = {
  _T("OLC Sprint"),
  _T("OLC FAI"),
  _T("OLC Classic"),
  _T("OLC League"),
  _T("OLC Plus"),
  _T("XContest"),
  _T("DHV-XC"),
  _T("SIS-AT"),
  _T("FFVV NetCoupe"),
  _T("DMSt"),
  _T("WeGlide FREE"),
  _T("WeGlide Distance"),
  _T("WeGlide FAI"),
  _T("WeGlide O&R"),
  _T("Charron"),
  _T("None"),
};

const TCHAR*
ContestToString(Contest contest) noexcept
{
  unsigned i = (unsigned)contest;
  return i < ARRAY_SIZE(contest_to_string)
    ? contest_to_string[i]
    : nullptr;
}
