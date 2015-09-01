/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "Util/StringAPI.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StringPointer.hxx"
#include "Util/AllocatedString.hxx"
#include "Util/Macros.hpp"

#include <windef.h> /* for MAX_PATH */

#include <algorithm>

bool
ProfileMap::GetPath(const char *key, TCHAR *value) const
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return false;

  if (StringIsEmpty(buffer))
    return false;

  ExpandLocalPath(value, buffer);
  return true;
}

bool
ProfileMap::GetPathIsEqual(const char *key, const TCHAR *value) const
{
  TCHAR saved[MAX_PATH];
  if (!GetPath(key, saved))
    return false;

  return StringIsEqual(saved, value);
}

gcc_pure
static const TCHAR *
BackslashBaseName(const TCHAR *p)
{
  if (DIR_SEPARATOR != '\\') {
    const auto *backslash = StringFindLast(p, _T('\\'));
    if (backslash != NULL)
      p = backslash + 1;
  }

  return BaseName(p);
}

#ifdef _UNICODE

AllocatedString<TCHAR>
ProfileMap::GetPathBase(const char *key) const
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return nullptr;

  const TCHAR *base = BackslashBaseName(buffer);
  if (base == nullptr)
    return nullptr;

  return AllocatedString<TCHAR>::Duplicate(base);
}

#else

StringPointer<TCHAR>
ProfileMap::GetPathBase(const char *key) const
{
  const auto *path = Get(key);
  if (path != nullptr)
    path = BackslashBaseName(path);

  return path;
}

#endif

void
ProfileMap::SetPath(const char *key, const TCHAR *value)
{
  TCHAR path[MAX_PATH];

  if (StringIsEmpty(value))
    path[0] = '\0';
  else {
    CopyString(path, value, MAX_PATH);
    ContractLocalPath(path);
  }

  Set(key, path);
}
