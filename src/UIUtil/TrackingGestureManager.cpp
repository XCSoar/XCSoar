// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#include "TrackingGestureManager.hpp"
#include "Math/FastMath.hpp"

#include <algorithm>
#include <cassert>

/**
 * Maximum number of trail points; the oldest point is dropped when
 * this limit is reached.
 */
static constexpr std::size_t MAX_TRAIL_POINTS = 512;

/** Divisor for deriving the trail spacing from the gesture threshold */
static constexpr int TRAIL_SPACING_DIVISOR = 10;

bool
TrackingGestureManager::Update(PixelPoint p)
{
  assert(points.size() >= 2);

  points.back() = p;

  const bool threshold_reached = GestureManager::Update(p);

  /* the trail is sampled independently of (and much denser than) the
     gesture detection threshold */
  const auto d = p - points[points.size() - 2];
  if (compare_squared(d.x, d.y, trail_spacing) == 1) {
    if (points.size() >= MAX_TRAIL_POINTS)
      points.erase(points.begin());

    points.emplace_back(p);
  }

  return threshold_reached;
}

void
TrackingGestureManager::Start(PixelPoint p, int threshold)
{
  points.clear();

  // Start point
  points.emplace_back(p);

  // Next point that is changed by Update()
  points.emplace_back(p);

  trail_spacing = std::max(MIN_TRAIL_SPACING,
                           threshold / TRAIL_SPACING_DIVISOR);

  GestureManager::Start(p, threshold);
}

const char*
TrackingGestureManager::Finish()
{
  points.clear();
  return GestureManager::Finish();
}
