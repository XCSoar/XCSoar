// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ByteSizeFormatter.hpp"
#include "util/Macros.hpp"
#include "util/StringFormat.hpp"

#include <cassert>

void
FormatByteSize(char *buffer, size_t size, unsigned long bytes, bool simple)
{
  assert(buffer != NULL);
  assert(size >= 8);

  static const char *const units[] = { _T("B"), _T("KB"), _T("MB"), _T("GB") };
  static const char *const simple_units[] = { _T("B"), _T("K"), _T("M"), _T("G") };

  double value = bytes;

  unsigned i = 0;
  for (; value >= 1024 && i < ARRAY_SIZE(units)-1; i++, value /= 1024);

  const char *unit = simple ? simple_units[i] : units[i];

  const char *format;
  if (value >= 100 || i == 0)
    format = simple ? _T("%.0f%s") : _T("%.0f %s");
  else if (value >= 10)
    format = simple ? _T("%.1f%s") : _T("%.1f %s");
  else
    format = simple ? _T("%.1f%s") : _T("%.2f %s");

  StringFormat(buffer, size, format, value, unit);
}
