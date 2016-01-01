/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ByteSizeFormatter.hpp"
#include "Util/Macros.hpp"
#include "Util/StringFormat.hpp"

#include <assert.h>

void
FormatByteSize(TCHAR *buffer, size_t size, unsigned long bytes, bool simple)
{
  assert(buffer != NULL);
  assert(size >= 8);

  static const TCHAR *const units[] = { _T("B"), _T("KB"), _T("MB"), _T("GB") };
  static const TCHAR *const simple_units[] = { _T("B"), _T("K"), _T("M"), _T("G") };

  double value = bytes;

  unsigned i = 0;
  for (; value >= 1024 && i < ARRAY_SIZE(units)-1; i++, value /= 1024);

  const TCHAR *unit = simple ? simple_units[i] : units[i];

  const TCHAR *format;
  if (value >= 100 || i == 0)
    format = simple ? _T("%.0f%s") : _T("%.0f %s");
  else if (value >= 10)
    format = simple ? _T("%.1f%s") : _T("%.1f %s");
  else
    format = simple ? _T("%.1f%s") : _T("%.2f %s");

  StringFormat(buffer, size, format, value, unit);
}
