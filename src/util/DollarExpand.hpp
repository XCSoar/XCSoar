// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StringAPI.hxx"
#include "TruncateString.hpp"
#include "tstring_view.hxx"

#include <concepts>
#include <span>

#include <tchar.h>

/**
 * Expand variable references in the "$(NAME)" style from the source
 * string and copy the result to the destination buffer.  If the
 * buffer is too small, then the output is truncated silently.
 */
void
DollarExpand(const char *src, std::span<char> dest,
             std::invocable<tstring_view> auto lookup_function) noexcept
{
  while (true) {
    auto dollar = StringFind(src, _T("$("));
    if (dollar == nullptr)
      break;

    auto name_start = dollar + 2;
    auto closing = StringFind(name_start, _T(')'));
    if (closing == nullptr)
      break;

    const tstring_view name(name_start, closing - name_start);

    const std::size_t prefix_size = dollar - src;
    if (prefix_size >= dest.size())
      break;

    /* copy the portion up to the dollar to the destination buffer */

    std::copy(src, dollar, dest.begin());
    dest = dest.subspan(prefix_size);
    src = closing + 1;

    /* look up the name and copy the result to the destination
       buffer */

    const char *const expansion = lookup_function(name);
    if (expansion != nullptr) {
      const tstring_view ex{expansion};
      if (ex.size() >= dest.size())
        break;

      std::copy(ex.begin(), ex.end(), dest.begin());
      dest = dest.subspan(ex.size());
    }
  }

  /* copy the remainder */
  CopyTruncateString(dest.data(), dest.size(), src);
}
