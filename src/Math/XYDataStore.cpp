// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XYDataStore.hpp"

void
XYDataStore::StoreReset() noexcept
{
  sum_n = 0;
  sum_xw = 0.;
  sum_yw = 0.;
  sum_weights = 0.;
  slots.clear();
}

void
XYDataStore::StoreAdd(double x, double y, double weight) noexcept
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
XYDataStore::StoreRemove(const unsigned i) noexcept
{
  assert(i< sum_n);
  const auto &pt = slots[i];

  // Remove weighted point
  const auto weight = pt.weight;

  sum_weights -= weight;

  sum_xw -= pt.x * weight;
  sum_yw -= pt.y * weight;

  slots.remove(i);
  --sum_n;
}
