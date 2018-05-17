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

#include "XYDataStore.hpp"

void
XYDataStore::StoreReset()
{
  sum_n = 0;
  sum_xw = 0.;
  sum_yw = 0.;
  sum_weights = 0.;
  slots.clear();
}

void
XYDataStore::StoreAdd(double x, double y, double weight)
{
  // Update maximum/minimum values
  if (IsEmpty() || y > y_max)
    y_max = y;

  if (IsEmpty() || y < y_min)
    y_min = y;

  if (IsEmpty() || x > x_max)
    x_max = x;

  if (IsEmpty() || x < x_min)
    x_min = x;

  // Add point
  // TODO code: really should have a circular buffer here
  if (!slots.full())
    slots.append() = Slot(x, y, weight);

  ++sum_n;

  // Add weighted point
  sum_weights += weight;

  sum_xw += x * weight;
  sum_yw += y * weight;
}

void
XYDataStore::StoreRemove(const unsigned i)
{
  assert(i< sum_n);
  const auto &pt = slots[i];

  // Remove weighted point
  auto weight = 1;
#ifdef LEASTSQS_WEIGHT_STORE
  weight = pt.weight;
#endif

  sum_weights -= weight;

  sum_xw -= pt.x * weight;
  sum_yw -= pt.y * weight;

  slots.remove(i);
  --sum_n;
}
