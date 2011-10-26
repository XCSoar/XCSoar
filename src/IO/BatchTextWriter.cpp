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

#include "BatchTextWriter.hpp"
#include "TextWriter.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

bool
BatchTextWriter::writeln(const char *line)
{
  if (buffer.IsFull() && !flush())
    return false;

  buffer.Append() = line;
  return true;
}

#ifdef _UNICODE
bool
BatchTextWriter::writeln(const TCHAR *line)
{
  if (buffer.IsFull() && !flush())
    return false;

  size_t wide_length = _tcslen(line);
  char narrow[wide_length * 4 + 1];
  int narrow_length = ::WideCharToMultiByte(CP_UTF8, 0, line, wide_length,
                                            narrow, sizeof(narrow),
                                            NULL, NULL);
  if (narrow_length == 0 && wide_length > 0)
    return false;

  buffer.Append().assign(narrow, narrow_length);
  return true;
}
#endif

bool
BatchTextWriter::flush()
{
  if (buffer.IsEmpty())
    return true;

  TextWriter writer(path.c_str(), append);
  if (writer.error())
    return false;

  /* the following flush() calls will append */
  append = true;

  for (unsigned i = 0; i < buffer.Length(); ++i)
    if (!writer.writeln(buffer[i].c_str()))
      return false;

  buffer.Clear();

  return true;
}
