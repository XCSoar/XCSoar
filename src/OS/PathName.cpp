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

#include "OS/PathName.hpp"
#include "Util/StringCompare.hxx"
#include "Util/StringAPI.hxx"

gcc_pure
static const TCHAR *
LastSeparator(const TCHAR *path)
{
  const auto *p = StringFindLast(path, _T('/'));
#ifdef WIN32
  const auto *backslash = StringFindLast(path, _T('\\'));
  if (p == nullptr || backslash > p)
    p = backslash;
#endif
  return p;
}

gcc_pure
static TCHAR *
LastSeparator(TCHAR *path)
{
  return const_cast<TCHAR *>(LastSeparator((const TCHAR *)path));
}

void
ReplaceBaseName(TCHAR *path, const TCHAR *new_base)
{
  TCHAR *q = LastSeparator(path);
  if (q != nullptr)
    ++q;
  else
    q = path;
  _tcscpy(q, new_base);
}

bool
MatchesExtension(const TCHAR *filename, const TCHAR *extension)
{
  size_t filename_length = _tcslen(filename);
  size_t extension_length = _tcslen(extension);

  return filename_length > extension_length &&
    StringIsEqualIgnoreCase(filename + filename_length - extension_length,
                            extension);
}

#ifdef _UNICODE

bool
MatchesExtension(const char *filename, const char *extension)
{
  size_t filename_length = strlen(filename);
  size_t extension_length = strlen(extension);

  return filename_length > extension_length &&
    StringIsEqualIgnoreCase(filename + filename_length - extension_length,
                            extension);
}

#endif
