// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoutePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "Geo/Flat/FlatGeoPoint.hpp"
#include "util/Macros.hpp"

GlideResult
RoutePolar::SolveTask(const GlideSettings &settings,
                      const GlidePolar& glide_polar,
                       const SpeedVector& wind,
                       const Angle theta, const bool glide) const
{
  const MacCready mac_cready(settings, glide_polar);
  GlideState task(GeoVector(1, theta), 0, 0, wind);
  return glide
    ? mac_cready.SolveStraight(task)
    : mac_cready.Solve(task);
}

void
RoutePolar::Initialise(const GlideSettings &settings, const GlidePolar& polar,
                       const SpeedVector& wind,
                       const bool is_glide)
{
  static constexpr Angle ang_step = Angle::FullCircle() / ROUTEPOLAR_POINTS;

  Angle ang = Angle::QuarterCircle();
  for (unsigned i = 0; i < ROUTEPOLAR_POINTS; ++i, ang -= ang_step) {
    GlideResult res = SolveTask(settings, polar, wind, ang, is_glide);
    if (res.IsOk()) {
      RoutePolarPoint point(res.time_elapsed.count(), res.height_glide);
      points[i] = point;
    } else
      points[i].valid = false;
  }
}

static constexpr FlatGeoPoint index_to_point[] = {
  {128, 0},
  {126, 16},
  {123, 32},
  {118, 48},
  {111, 62},
  {102, 76},
  {91, 89},
  {79, 100},
  {66, 109},
  {51, 117},
  {36, 122},
  {20, 126},
  {4, 127},
  {-12, 127},
  {-28, 124},
  {-44, 120},
  {-59, 113},
  {-73, 104},
  {-86, 94},
  {-97, 82},
  {-107, 69},
  {-115, 55},
  {-121, 40},
  {-125, 24},
  {-127, 8},
  {-127, -8},
  {-125, -24},
  {-121, -40},
  {-115, -55},
  {-107, -69},
  {-97, -82},
  {-86, -94},
  {-73, -104},
  {-59, -113},
  {-44, -120},
  {-28, -124},
  {-12, -127},
  {4, -127},
  {20, -126},
  {36, -122},
  {51, -117},
  {66, -109},
  {79, -100},
  {91, -89},
  {102, -76},
  {111, -62},
  {118, -48},
  {123, -32},
  {126, -16},
};

FlatGeoPoint
RoutePolar::IndexToDXDY(const int index)
{
  static_assert(ARRAY_SIZE(index_to_point) == ROUTEPOLAR_POINTS,
                "Wrong array size");

  return index_to_point[index];
}
