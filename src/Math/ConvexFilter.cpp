// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math/ConvexFilter.hpp"

void
ConvexFilter::UpdateConvex(double x, double y, int csign) noexcept
{
  // ignore if coincident or back in time
  if (!IsEmpty() && x <= GetMaxX())
    return;

  Update(x, y, 1);

  // check pruning of previous points

  while (GetCount() > 2) {
    const unsigned n = GetCount();
    const auto &s = GetSlots();
    const auto &next = s[n - 1];
    const auto &prev = s[n - 3];
    const double m = (next.y-prev.y)/(next.x-prev.x);
    const auto &cur = s[n - 2];
    const double y_est = (cur.x - prev.x)*m + prev.y;

    // if this point doesn't need pruning, neither will predecessors
    if (csign*(cur.y - y_est) > 0)
      return;

    // prune this point, and continue checking previous points for
    // pruning
    Remove(n - 2);
  }
}
