// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hxx"
#include "StringAPI.hxx"
#include "TruncateString.hpp"
#include "tstring_view.hxx"

#include <tchar.h>

/**
 * Expand variable references in the "$(NAME)" style from the source
 * string and copy the result to the destination buffer.  If the
 * buffer is too small, then the output is truncated silently.
 */
void
DollarExpand(const TCHAR *src, TCHAR *dest, size_t dest_size,
             Invocable<tstring_view> auto lookup_function) noexcept
{
  const TCHAR *const dest_end = dest + dest_size;

  while (true) {
    auto dollar = StringFind(src, _T("$("));
    if (dollar == nullptr)
      break;

    auto name_start = dollar + 2;
    auto closing = StringFind(name_start, _T(')'));
    if (closing == nullptr)
      break;

    const tstring_view name(name_start, closing - name_start);

    dest_size = dest_end - dest;
    if (size_t(dollar - src) >= dest_size)
      break;

    /* copy the portion up to the dollar to the destination buffer */

    dest = std::copy(src, dollar, dest);
    src = closing + 1;

    /* look up the name and copy the result to the destination
       buffer */

    const TCHAR *const expansion = lookup_function(name);
    if (expansion != nullptr) {
      dest_size = dest_end - dest;
      dest = CopyTruncateString(dest, dest_size, expansion);
    }
  }

  /* copy the remainder */
  dest_size = dest_end - dest;
  CopyTruncateString(dest, dest_size, src);
}
