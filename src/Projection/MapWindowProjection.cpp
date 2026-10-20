// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindowProjection.hpp"
#include "Waypoint/Waypoint.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Globals.hpp"
#endif

#include <algorithm> // for std::clamp()
#include <cassert>

static constexpr unsigned ScaleList[] = {
  100,
  200,
  300,
  500,
  1000,
  2000,
  3000,
  5000,
  10000,
  20000,
  30000,
  50000,
  75000,
  100000,
  150000,
  200000,
  300000,
  500000,
  1000000,
};

static constexpr unsigned ScaleListCount = std::size(ScaleList);

namespace {

/**
 * Largest #GetMapScale() at which this waypoint is still drawn (aligned with
 * #ScaleList steps; lower threshold = hidden sooner when zooming out).
 */
static double
WaypointDrawMaxScale(const Waypoint &wp) noexcept
{
  if (wp.IsLandable())
    return 20000;
  if (wp.type == Waypoint::Type::OBSTACLE)
    return 5000;
  return 10000;
}

} // namespace

bool
MapWindowProjection::WaypointInScaleFilter(const Waypoint &way_point) const noexcept
{
  return GetMapScale() <= WaypointDrawMaxScale(way_point);
}

double
MapWindowProjection::CalculateMapScale(unsigned scale) const noexcept
{
  assert(scale < ScaleListCount);
  /* ScaleList is the scale-bar width in metres.  Divide by the map
     pixel width, not Layout::Scale(width): that UI factor grows with
     DPI and was capping wheel zoom at about 170 km on a 1400 px
     window. */
  return double(ScaleList[scale]) *
    GetMapResolutionFactor() / GetScreenSize().width;
}

/**
 * Determine the effective number of usable entries in the ScaleList.
 * May be reduced by OpenGL::max_map_scale to work around GPU driver
 * bugs.
 */
static unsigned
EffectiveScaleListCount() noexcept
{
#ifdef ENABLE_OPENGL
  if (OpenGL::max_map_scale > 0) {
    for (unsigned i = 0; i < ScaleListCount; i++)
      if (ScaleList[i] > OpenGL::max_map_scale)
        return i;
  }
#endif

  return ScaleListCount;
}

double
MapWindowProjection::LimitMapScale(const double value) const noexcept
{
  return HaveScaleList() ? CalculateMapScale(FindMapScale(value)) : value;
}

double
MapWindowProjection::StepMapScale(const double scale, int Step) const noexcept
{
  int i = FindMapScale(scale) + Step;
  i = std::clamp(i, 0, (int)EffectiveScaleListCount() - 1);
  return CalculateMapScale(i);
}

unsigned
MapWindowProjection::FindMapScale(const double Value) const noexcept
{
  const unsigned effective_count = EffectiveScaleListCount();

  const unsigned desired_scale = (unsigned)
    (Value * double(GetScreenSize().width)
     / double(GetMapResolutionFactor()));

  unsigned i;
  for (i = 0; i < effective_count; i++) {
    if (desired_scale < ScaleList[i]) {
      if (i == 0)
        return 0;

      return i - (desired_scale < (ScaleList[i] + ScaleList[i - 1]) / 2);
    }
  }

  return effective_count - 1;
}

void
MapWindowProjection::SetFreeMapScale(double x) noexcept
{
#ifdef ENABLE_OPENGL
  if (OpenGL::max_map_scale > 0)
    x = std::min(x, double(OpenGL::max_map_scale));
#endif

  SetScale(double(GetMapResolutionFactor()) / x);
}

void
MapWindowProjection::SetMapScale(const double x) noexcept
{
  SetScale(double(GetMapResolutionFactor()) / LimitMapScale(x));
}
