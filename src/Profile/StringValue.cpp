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

#include "Map.hpp"
#include "Util/UTF8.hpp"
#include "Util/TruncateString.hpp"
#include "Util/Macros.hpp"

#ifdef _UNICODE
#include "Util/Macros.hpp"

#include <windows.h>
#endif

bool
ProfileMap::Get(const char *key, TCHAR *value, size_t max_size) const
{
  const char *src = Get(key);
  if (src == nullptr) {
    value[0] = _T('\0');
    return false;
  }

#ifdef _UNICODE
  int result = MultiByteToWideChar(CP_UTF8, 0, src, -1,
                                   value, max_size);
  return result > 0;
#else
  if (!ValidateUTF8(src))
    return false;

  CopyTruncateString(value, max_size, src);
  return true;
#endif
}

#ifdef _UNICODE

void
ProfileMap::Set(const char *key, const TCHAR *value)
{
  char buffer[MAX_PATH];
  int length = WideCharToMultiByte(CP_UTF8, 0, value, -1,
                                   buffer, ARRAY_SIZE(buffer),
                                   nullptr, nullptr);
  if (length <= 0)
    return;

  Set(key, buffer);
}

#endif
