/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#ifndef DOLLAR_EXPAND_HPP
#define DOLLAR_EXPAND_HPP

/** \file */

#include "StringAPI.hxx"
#include "TruncateString.hpp"

#include <tchar.h>

/**
 * Expand variable references in the "$(NAME)" style from the source
 * string and copy the result to the destination buffer.  If the
 * buffer is too small, then the output is truncated silently.
 */
template<typename F>
void
DollarExpand(const TCHAR *src, TCHAR *dest, size_t dest_size,
             F &&lookup_function)
{
  const TCHAR *const dest_end = dest + dest_size;

  while (true) {
    auto dollar = StringFind(src, _T("$("));
    if (dollar == nullptr)
      break;

    auto name = dollar + 2;
    auto closing = StringFind(name, _T(')'));
    if (closing == nullptr)
      break;

    dest_size = dest_end - dest;
    if (size_t(dollar - src) >= dest_size)
      break;

    /* copy the portion up to the dollar to the destination buffer */

    dest = std::copy(src, dollar, dest);
    src = closing + 1;

    /* copy the name to the destination buffer so we can
       null-terminate it for the callback */

    const size_t name_size = closing - name;
    dest_size = dest_end - dest;
    if (name_size >= dest_size)
      break;

    *std::copy(name, closing, dest) = 0;

    /* look it up and copy the result to the destination buffer */

    const TCHAR *const expansion = lookup_function(dest);
    if (expansion != nullptr) {
      dest_size = dest_end - dest;
      dest = CopyTruncateString(dest, dest_size, expansion);
    }
  }

  /* copy the remainder */
  dest_size = dest_end - dest;
  CopyTruncateString(dest, dest_size, src);
}

#endif
