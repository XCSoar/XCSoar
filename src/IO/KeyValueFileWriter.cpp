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

#include "KeyValueFileWriter.hpp"
#include "BufferedOutputStream.hxx"
#include "Util/Macros.hpp"

#include <assert.h>
#include <string.h>

#ifdef _UNICODE
#include <windows.h>
#endif

void
KeyValueFileWriter::Write(const char *key, const char *value)
{
  assert(key != nullptr);
  assert(value != nullptr);

  // does it contain invalid characters?
  if (strpbrk(value, "\r\n\"") != nullptr)
    // -> write ="" to the output file an continue with the next subkey
    value = "";

  // write the value to the output file
  os.Format("%s=\"%s\"\n", key, value);
}

#ifdef _UNICODE

void
KeyValueFileWriter::Write(const char *key, const TCHAR *value)
{
  char buffer[1024];
  int result = WideCharToMultiByte(CP_UTF8, 0, value, -1,
                                   buffer, ARRAY_SIZE(buffer),
                                   nullptr, nullptr);
  if (result > 0)
    Write(key, buffer);
}

#endif
