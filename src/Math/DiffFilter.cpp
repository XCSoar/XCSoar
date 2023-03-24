// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DiffFilter.hpp"
#include "Constants.hpp"

#include <algorithm>

void
DiffFilter::Reset(const double x0, const double y0) noexcept
{
  for (unsigned i = 0; i < x.size(); i++)
    x[i] = x0 - y0 * i;
}

double
DiffFilter::Update(const double x0) noexcept
{
  std::copy_backward(x.cbegin(), std::prev(x.cend()), x.end());
  x.front() = x0;

  /// @note not sure why need to divide by pi/2 here
  return ((x.back() - x.front()) / 16 + x[2] - x[4]) / M_PI_2;
}

