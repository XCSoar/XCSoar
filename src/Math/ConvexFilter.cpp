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

#include "Math/ConvexFilter.hpp"

void
ConvexFilter::UpdateConvex(double x, double y, int csign) noexcept
{
  // ignore if coincident or back in time
  if (!IsEmpty() && x <= x_max)
    return;

  Update(x, y, 1);

  // check pruning of previous points

  while (sum_n > 2) {
    const Slot& next = slots[sum_n-1];
    const Slot& prev = slots[sum_n-3];
    const double m = (next.y-prev.y)/(next.x-prev.x);
    const Slot& cur = slots[sum_n-2];
    const double y_est = (cur.x - prev.x)*m + prev.y;

    // if this point doesn't need pruning, neither will predecessors
    if (csign*(cur.y - y_est) > 0)
      return;

    // prune this point, and continue checking previous points for
    // pruning
    Remove(sum_n-2);
  }
}

double
ConvexFilter::GetLastY() const noexcept
{
  assert(!IsEmpty());

  return slots[sum_n-1].y;
}
