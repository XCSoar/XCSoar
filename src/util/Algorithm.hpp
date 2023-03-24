// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

/**
 * @return true if predicate() returns true for at least one item in
 * the range
 */
template<class I, typename P>
static inline bool
ExistsIf(I first, I last, P predicate)
{
  for (I i = first; i != last; ++i)
    if (predicate(*i))
      return true;

  return false;
}
