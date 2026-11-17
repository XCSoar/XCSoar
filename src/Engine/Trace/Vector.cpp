// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vector.hpp"
#include "Point.hpp"
#include "Geo/GeoBounds.hpp"

#include <algorithm>

void
TracePointVector::ScanBounds(GeoBounds &bounds) const noexcept
{
  for (const auto &i : *this)
    bounds.Extend(i.GetLocation());
}

std::pair<double, double>
TracePointVector::GetVarioPercentiles(const double share) const
{
  std::vector<double> vario;
  vario.reserve(size());
  for (const auto &i : *this)
    vario.push_back(i.GetVario());

  /* the number of readings left out at each end */
  const std::size_t n = std::min(std::size_t(share * vario.size()),
                                 (vario.size() - 1) / 2);
  const std::size_t low = n, high = vario.size() - 1 - n;
  std::nth_element(vario.begin(), vario.begin() + high, vario.end());
  /* the first partition left everything up to high in front */
  std::nth_element(vario.begin(), vario.begin() + low,
                   vario.begin() + high);
  return {vario[low], vario[high]};
}
