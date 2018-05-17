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
#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "Compatibility/path.h"
#include "Util/StringAPI.hxx"
#include "Util/StringCompare.hxx"
#include "Util/StringPointer.hxx"
#include "Util/Macros.hpp"

#ifdef _UNICODE
#include "Util/AllocatedString.hxx"
#endif

#include <windef.h> /* for MAX_PATH */

AllocatedPath
ProfileMap::GetPath(const char *key) const
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return nullptr;

  if (StringIsEmpty(buffer))
    return nullptr;

  return ExpandLocalPath(Path(buffer));
}

bool
ProfileMap::GetPathIsEqual(const char *key, Path value) const
{
  const auto saved_value = GetPath(key);
  if (saved_value.IsNull())
    return false;

  return saved_value == value;
}

gcc_pure
static Path
BackslashBaseName(const TCHAR *p)
{
  if (DIR_SEPARATOR != '\\') {
    const auto *backslash = StringFindLast(p, _T('\\'));
    if (backslash != NULL)
      p = backslash + 1;
  }

  return Path(p).GetBase();
}

#ifdef _UNICODE

AllocatedString<TCHAR>
ProfileMap::GetPathBase(const char *key) const
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return nullptr;

  const TCHAR *base = BackslashBaseName(buffer).c_str();
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
    path = BackslashBaseName(path).c_str();

  return path;
}

#endif

void
ProfileMap::SetPath(const char *key, Path value)
{
  if (value.IsNull() || StringIsEmpty(value.c_str()))
    Set(key, _T(""));
  else {
    const auto contracted = ContractLocalPath(value);
    if (contracted != nullptr)
      value = contracted;

    Set(key, value.c_str());
  }
}
