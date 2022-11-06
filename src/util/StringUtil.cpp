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

#include "StringUtil.hpp"
#include "CharUtil.hxx"
#include "Compiler.h"

#include <algorithm>

char *
CopyString(char *gcc_restrict dest, size_t dest_size,
           std::string_view src) noexcept
{
  if (src.size() >= dest_size)
    src = src.substr(0, dest_size -1);

  char *p = std::copy(src.begin(), src.end(), dest);
  *p = '\0';
  return p;
}

char *
NormalizeSearchString(char *gcc_restrict dest,
                      std::string_view src) noexcept
{
  char *retval = dest;

  for (const auto ch : src)
    if (IsAlphaNumericASCII(ch))
      *dest++ = ToUpperASCII(ch);

  *dest = '\0';

  return retval;
}
