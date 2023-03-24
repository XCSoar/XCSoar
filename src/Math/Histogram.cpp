// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Histogram.hpp"
#include "Util.hpp"

#include <algorithm>
#include <cassert>

inline Histogram::size_type
Histogram::SlotNumber(double x) const noexcept
{
  x -= b;
  if (x <= 0)
    return 0;

  size_type i = uround(m * x);
  if (i >= NUM_SLOTS)
    i = NUM_SLOTS - 1;

  return i;
}

inline void
Histogram::IncrementSlot(const size_type i, const double mag) noexcept
{
  slots[i].y += mag;
  y_max = std::max(slots[i].y, y_max);
}

void
Histogram::UpdateHistogram(double x) noexcept
{
  const size_type i = SlotNumber(x);

  double mag = 1;

  if (i > 0) {
    IncrementSlot(i - 1, SPREAD);
    mag -= SPREAD;
  }

  if (i < NUM_SLOTS - 1) {
    IncrementSlot(i + 1, SPREAD);
    mag -= SPREAD;
  }

  // remainder goes to actual slot
  IncrementSlot(i, mag);

  n_pts++;

  // update range
  x_min = std::min(x, x_min);
  x_max = std::max(x, x_max);
}

void
Histogram::Reset(double minx, double maxx) noexcept
{
  assert(maxx > minx);
  b = minx;
  m = (NUM_SLOTS - 1) / (maxx - minx);

  const double delta_x = 1 / m;
  double x = minx;
  for (auto &i : slots) {
    i = {x, 0.};
    x += delta_x;
  }

  n_pts = 0;
  x_min = 0;
  x_max = 0;
  y_max = 0;
}

void
Histogram::Clear() noexcept
{
  for (auto &i : slots)
    i.y = 0;

  n_pts = 0;
  x_min = 0;
  x_max = 0;
  y_max = 0;
}

double
Histogram::GetPercentile(const double p) const noexcept
{
  assert(p>= 0);
  assert(p<= 1);

  const double np = n_pts*p;
  double acc_n = 0;
  for (unsigned i = 0; i + 1 < NUM_SLOTS; ++i) {
    if (slots[i].y > np - acc_n) {
      const double residual = (np - acc_n) / slots[i].y;
      return slots[i+1].x * residual + slots[i].x * (1 - residual) - 0.5 / m;
    }
    acc_n += slots[i].y;
  }

  // return mid point
  return b + (NUM_SLOTS - 1) / (2 * m);
}
