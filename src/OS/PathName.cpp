/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "OS/PathName.hpp"
#include "Util/StringUtil.hpp"

#include <string.h>
#include <algorithm>

static const TCHAR *
LastSeparator(const TCHAR *path)
{
  const TCHAR *p = _tcsrchr(path, _T('/'));
#ifdef WIN32
  const TCHAR *backslash = _tcsrchr(path, _T('\\'));
  if (p == NULL || backslash > p)
    p = backslash;
#endif
  return p;
}

const TCHAR *
BaseName(const TCHAR *path)
{
  const TCHAR *p = LastSeparator(path);
  if (p != NULL)
    path = p + 1;

  if (string_is_empty(path))
    return NULL;

  return path;
}

const TCHAR *
DirName(const TCHAR *path, TCHAR *buffer)
{
  const TCHAR *p = LastSeparator(path);
  if (p == NULL || p == path)
    return _T(".");

  TCHAR *end = std::copy(path, p, buffer);
  *end = _T('\0');
  return buffer;
}
