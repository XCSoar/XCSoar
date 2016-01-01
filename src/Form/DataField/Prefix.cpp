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

#include "Prefix.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringCompare.hxx"

const TCHAR *
PrefixDataField::GetAsDisplayString() const
{
  const TCHAR *s = DataFieldString::GetAsDisplayString();
  if (StringIsEmpty(s))
    s = _T("*");
  return s;
}

void
PrefixDataField::Inc()
{
  const TCHAR *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const TCHAR current = GetAsString()[0];
  const TCHAR *p = current != _T('\0')
    ? StringFind(chars, current)
    : nullptr;

  TCHAR next;
  if (p == nullptr)
    next = chars[0];
  else
    next = p[1];

  const TCHAR new_value[2] = { next, _T('\0') };
  SetAsString(new_value);
}

void
PrefixDataField::Dec()
{
  const TCHAR *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const TCHAR current = GetAsString()[0];

  TCHAR next;
  if (current == _T('\0'))
    next = chars[_tcslen(chars) - 1];
  else {
    const TCHAR *p = current != _T('\0')
      ? StringFind(chars, current)
      : nullptr;

    if (p > chars)
      next = p[-1];
    else
      next = _T('\0');
  }

  const TCHAR new_value[2] = { next, _T('\0') };
  SetAsString(new_value);
}
