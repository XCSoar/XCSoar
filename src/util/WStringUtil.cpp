/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "WStringUtil.hpp"
#include "WStringAPI.hxx"
#include "WStringCompare.hxx"
#include "WCharUtil.hxx"

#include <algorithm>

wchar_t *
CopyString(wchar_t *gcc_restrict dest, const wchar_t *gcc_restrict src,
           size_t size)
{
  size_t length = StringLength(src);
  if (length >= size)
    length = size - 1;

  wchar_t *p = std::copy_n(src, length, dest);
  *p = L'\0';
  return p;
}

wchar_t *
NormalizeSearchString(wchar_t *gcc_restrict dest,
                      const wchar_t *gcc_restrict src)
{
  wchar_t *retval = dest;

  for (; !StringIsEmpty(src); ++src)
    if (IsAlphaNumericASCII(*src))
      *dest++ = ToUpperASCII(*src);

  *dest = L'\0';

  return retval;
}
