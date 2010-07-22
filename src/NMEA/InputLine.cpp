/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "NMEA/InputLine.hpp"

#include <assert.h>
#include <string.h>

static const TCHAR *
end_of_line(const TCHAR *line)
{
  const TCHAR *asterisk = _tcschr(line, '*');
  return asterisk != NULL
    ? asterisk
    : line + _tcslen(line);
}

NMEAInputLine::NMEAInputLine(const TCHAR *line)
  :data(line), end(end_of_line(line)) {
}

size_t
NMEAInputLine::skip()
{
  const TCHAR *comma = _tcschr(data, ',');
  if (comma != NULL && comma < end) {
    size_t length = comma - data;
    data = comma + 1;
    return length;
  } else {
    size_t length = end - data;
    data = end;
    return length;
  }
}

TCHAR
NMEAInputLine::read_first_char()
{
  TCHAR ch = *data;
  return skip() > 0 ? ch : _T('\0');
}

void
NMEAInputLine::read(TCHAR *dest, size_t size)
{
  const TCHAR *src = data;
  size_t length = skip();
  if (length >= size)
    length = size - 1;
  _tcsncpy(dest, src, length);
  dest[length] = _T('\0');
}

bool
NMEAInputLine::read_compare(const TCHAR *value)
{
  size_t length = _tcslen(value);
  TCHAR buffer[length + 2];
  read(buffer, length + 2);
  return _tcscmp(buffer, value) == 0;
}

static bool
is_end_of_line(TCHAR ch)
{
  return ch == _T('\0') || ch == _T('*');
}

long
NMEAInputLine::read(long default_value)
{
  TCHAR *endptr;
  long value = _tcstol(data, &endptr, 0);
  assert(endptr >= data && endptr <= end);
  if (is_end_of_line(*endptr)) {
    data = _T("");
    return value;
  } else if (*endptr == ',') {
    data = endptr + 1;
    return value;
  } else {
    data = endptr;
    skip();
    return default_value;
  }
}

double
NMEAInputLine::read(double default_value)
{
  TCHAR *endptr;
  double value = _tcstod(data, &endptr);
  assert(endptr >= data && endptr <= end);
  if (is_end_of_line(*endptr)) {
    data = _T("");
    return value;
  } else if (*endptr == ',') {
    data = endptr + 1;
    return value;
  } else {
    data = endptr;
    skip();
    return default_value;
  }
}

bool
NMEAInputLine::read_checked(double &value_r)
{
  TCHAR *endptr;
  double value = _tcstod(data, &endptr);
  assert(endptr >= data && endptr <= end);
  if (is_end_of_line(*endptr)) {
    data = _T("");
    value_r = value;
    return true;
  } else if (*endptr == ',') {
    data = endptr + 1;
    value_r = value;
    return true;
  } else {
    data = endptr;
    skip();
    return false;
  }
}

#ifdef FIXED_MATH

fixed
NMEAInputLine::read(fixed default_value)
{
  double value;
  return read_checked(value)
    ? fixed(value)
    : default_value;
}

bool
NMEAInputLine::read_checked(fixed &value_r)
{
  double value;
  if (read_checked(value)) {
    value_r = fixed(value);
    return true;
  } else
    return false;
}

#endif /* FIXED_MATH */

bool
NMEAInputLine::read_checked_compare(fixed &value_r, const TCHAR *string)
{
  fixed value;
  if (read_checked(value)) {
    if (read_compare(string)) {
      value_r = value;
      return true;
    } else
      return false;
  } else {
    skip();
    return false;
  }
}
