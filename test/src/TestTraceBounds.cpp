// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Trace/Trace.hpp"
#include "Engine/Trace/Vector.hpp"
#include "Geo/GeoBounds.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <chrono>

using namespace std::chrono;

/**
 * Build a long eastbound track, then query a tight box around the end.
 * Bounds-first GetPoints must return far fewer points than the full
 * spacing-thinned path, while still including a leg that crosses the box.
 */
static void
TestBoundsFirstQuery() noexcept
{
  Trace trace(seconds{0}, Trace::null_time, 8192);

  constexpr unsigned n_points = 2000;
  constexpr double step_deg = 0.001; /* ~111 m east per step */

  for (unsigned i = 0; i < n_points; ++i) {
    const GeoPoint loc(Angle::Degrees(8 + i * step_deg),
                       Angle::Degrees(48));
    const TracePoint point(loc, seconds{i * 2}, 1000, 0, 0);
    trace.push_back(point);
  }

  ok1(trace.size() > 1000);

  TracePointVector all_thinned;
  const GeoPoint end_loc = trace.back().GetLocation();
  /* Fine spacing: almost every stored point survives thinning. */
  trace.GetPoints(all_thinned, {}, end_loc, 1.);
  ok1(all_thinned.size() > 1000);

  /* Tight box around the end (~0.01° ≈ 1 km). */
  const GeoBounds box(
    GeoPoint(Angle::Degrees(8 + (n_points - 20) * step_deg),
             Angle::Degrees(48.005)),
    GeoPoint(Angle::Degrees(8 + (n_points - 1) * step_deg + 0.002),
             Angle::Degrees(47.995)));
  ok1(box.IsValid());
  ok1(box.IsInside(end_loc));

  TracePointVector bounded;
  trace.GetPoints(bounded, {}, box, end_loc, 1.);
  ok1(!bounded.empty());
  ok1(bounded.size() < all_thinned.size() / 10);
  ok1(bounded.size() < 80);

  /* Last point of the flight must be present. */
  ok1(bounded.back().GetTime() == trace.back().GetTime());

  /* A point well before the box must not appear. */
  bool early_found = false;
  const auto early_time = seconds{100};
  for (const auto &p : bounded) {
    if (p.GetTime() == early_time) {
      early_found = true;
      break;
    }
  }
  ok1(!early_found);
}

/**
 * Coarse min_distance used to drop the newest samples; the tip must
 * still be present (open leg / follow would otherwise rubber-band).
 */
static void
TestThinningKeepsLatestPoint() noexcept
{
  Trace trace(seconds{0}, Trace::null_time, 256);

  constexpr unsigned n_points = 40;
  constexpr double step_deg = 0.0001; /* ~11 m */

  for (unsigned i = 0; i < n_points; ++i) {
    const GeoPoint loc(Angle::Degrees(8 + i * step_deg),
                       Angle::Degrees(48));
    trace.push_back(TracePoint(loc, seconds{i * 2}, 1000, 0, 0));
  }

  const GeoPoint end_loc = trace.back().GetLocation();
  const GeoBounds box(
    GeoPoint(Angle::Degrees(7.9), Angle::Degrees(48.1)),
    GeoPoint(Angle::Degrees(8.1), Angle::Degrees(47.9)));

  TracePointVector bounded;
  /* 5 km: only first would survive a forward thin without keeping the tip. */
  trace.GetPoints(bounded, {}, box, end_loc, 5000.);
  ok1(!bounded.empty());
  ok1(bounded.front().GetTime() == seconds{0});
  ok1(bounded.back().GetTime() == trace.back().GetTime());
  ok1(bounded.size() == 2);
}

/**
 * Dropping the oldest sample must not retarget the whole thinning
 * lattice.  Kept times from the tip stay the same (follow / 10x replay).
 */
static void
TestThinningAnchoredAtTip() noexcept
{
  Trace trace(seconds{0}, Trace::null_time, 256);

  constexpr unsigned n_points = 40;
  constexpr double step_deg = 0.0005; /* ~55 m */

  for (unsigned i = 0; i < n_points; ++i) {
    const GeoPoint loc(Angle::Degrees(8 + i * step_deg),
                       Angle::Degrees(48));
    trace.push_back(TracePoint(loc, seconds{i * 2}, 1000, 0, 0));
  }

  const GeoPoint end_loc = trace.back().GetLocation();
  const GeoBounds box(
    GeoPoint(Angle::Degrees(7.9), Angle::Degrees(48.1)),
    GeoPoint(Angle::Degrees(8.1), Angle::Degrees(47.9)));

  TracePointVector full, dropped_first;
  trace.GetPoints(full, {}, box, end_loc, 200.);
  trace.GetPoints(dropped_first, seconds{2}, box, end_loc, 200.);

  ok1(!full.empty());
  ok1(!dropped_first.empty());
  ok1(full.back().GetTime() == trace.back().GetTime());
  ok1(dropped_first.back().GetTime() == trace.back().GetTime());

  size_t match = 0;
  while (match < full.size() && match < dropped_first.size() &&
         full[full.size() - 1 - match].GetTime() ==
           dropped_first[dropped_first.size() - 1 - match].GetTime())
    ++match;

  ok1(match >= std::min(full.size(), dropped_first.size()) - 1);
}

/**
 * A single long leg that crosses the viewport must keep both endpoints
 * (outside → outside with segment through the box).
 */
static void
TestCrossingLegKept() noexcept
{
  Trace trace(seconds{0}, Trace::null_time, 64);

  const GeoPoint west(Angle::Degrees(8.0), Angle::Degrees(48));
  const GeoPoint east(Angle::Degrees(8.2), Angle::Degrees(48));
  trace.push_back(TracePoint(west, seconds{0}, 1000, 0, 0));
  /* push_back enforces 2 s min delta */
  trace.push_back(TracePoint(east, seconds{10}, 1000, 0, 0));

  ok1(trace.size() == 2);

  const GeoBounds box(
    GeoPoint(Angle::Degrees(8.05), Angle::Degrees(48.05)),
    GeoPoint(Angle::Degrees(8.15), Angle::Degrees(47.95)));
  ok1(box.IsValid());
  ok1(!box.IsInside(west));
  ok1(!box.IsInside(east));

  TracePointVector bounded;
  const GeoPoint mid(Angle::Degrees(8.1), Angle::Degrees(48));
  trace.GetPoints(bounded, {}, box, mid, 1.);

  ok1(bounded.size() == 2);
  ok1(bounded.front().GetTime() == seconds{0});
  ok1(bounded.back().GetTime() == seconds{10});
}

int
main()
{
  plan_tests(9 + 7 + 4 + 5);
  TestBoundsFirstQuery();
  TestCrossingLegKept();
  TestThinningKeepsLatestPoint();
  TestThinningAnchoredAtTip();
  return exit_status();
}
