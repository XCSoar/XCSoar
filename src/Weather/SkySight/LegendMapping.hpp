// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"

#include <cmath>
#include <iterator>
#include <map>

namespace SkySight {

/**
 * Map a sample onto the SkySight legend.
 *
 * Uses floor coloring (each stop covers up to the next).  Values below the
 * first stop are transparent.  When the legend has both non-positive and
 * positive stops, the last non-positive stop's interval
 * [last_non_positive, first_positive) is also left transparent: that band is
 * near-zero background for bipolar overlays such as ridge lift / convergence,
 * and painting it opaque washes out the map.
 *
 * @return pointer into @p legend, or nullptr when the sample should stay
 *         transparent
 */
[[nodiscard]] inline const LegendColor *
FindLegendColor(const std::map<float, LegendColor> &legend,
                float value) noexcept
{
  if (legend.empty() || !std::isfinite(value))
    return nullptr;

  auto color = legend.upper_bound(value);
  if (color == legend.begin())
    return nullptr;

  --color;

  const auto first_positive = legend.upper_bound(0.f);
  if (first_positive != legend.end() &&
      first_positive != legend.begin()) {
    const auto last_non_positive = std::prev(first_positive);
    if (color == last_non_positive)
      return nullptr;
  }

  return &color->second;
}

} // namespace SkySight
