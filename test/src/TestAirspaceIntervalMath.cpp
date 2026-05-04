// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Airspace/AirspaceWarningInterval.hpp"
#include "TestUtil.hpp"

static constexpr GeoPoint
P(double lon, double lat) noexcept
{
  return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
}

static AirspaceWarningInterval
MakeIv(double e_dist, GeoPoint e_loc,
       double x_dist, GeoPoint x_loc) noexcept
{
  return {{e_dist, e_loc}, {x_dist, x_loc}};
}

int
main()
{
  plan_tests(31);

  /* arbitrary placeholder GeoPoints; only distances matter for
     most checks, but we use distinct points to verify that
     clip-start uses the cleared.exit location and clip-end uses
     the cleared.entry location. */
  const GeoPoint a = P(10.0, 50.0);
  const GeoPoint b = P(11.0, 50.0);

  /* Case 1: cleared after target, no overlap */
  {
    auto t = MakeIv(100, a, 200, b);
    SubtractInterval(t, MakeIv(300, a, 400, b));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 100));
    ok1(equals(t.exit.distance, 200));
  }

  /* Case 2: cleared before target, no overlap */
  {
    auto t = MakeIv(100, a, 200, b);
    SubtractInterval(t, MakeIv(0, a, 50, b));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 100));
    ok1(equals(t.exit.distance, 200));
  }

  /* Case 3: cleared clips start (entry moves to cleared.exit) */
  {
    auto t = MakeIv(100, a, 200, b);
    const GeoPoint c_exit = P(10.5, 50.0);
    SubtractInterval(t, MakeIv(50, a, 150, c_exit));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 150));
    ok1(t.entry.location == c_exit);
    ok1(equals(t.exit.distance, 200));
  }

  /* Case 4: cleared clips end (exit moves to cleared.entry) */
  {
    auto t = MakeIv(100, a, 200, b);
    const GeoPoint c_entry = P(10.6, 50.0);
    SubtractInterval(t, MakeIv(150, c_entry, 250, a));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 100));
    ok1(equals(t.exit.distance, 150));
    ok1(t.exit.location == c_entry);
  }

  /* Case 5: cleared fully covers target */
  {
    auto t = MakeIv(100, a, 200, b);
    SubtractInterval(t, MakeIv(50, a, 250, b));
    ok1(!t.IsValid());
  }

  /* Case 6: hole, nearest fragment >= 50m, keep nearest */
  {
    auto t = MakeIv(0, a, 1000, b);
    /* nearest fragment 0..400 (400m), farther 600..1000 (400m) */
    SubtractInterval(t, MakeIv(400, a, 600, b));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 0));
    ok1(equals(t.exit.distance, 400));
  }

  /* Case 7: hole, nearest fragment < 50m, keep farther */
  {
    auto t = MakeIv(0, a, 1000, b);
    /* nearest fragment 0..30 (30m, < tolerance), farther 50..1000 */
    SubtractInterval(t, MakeIv(30, a, 50, b));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 50));
    ok1(equals(t.exit.distance, 1000));
  }

  /* Case 8: sequential subtraction (two cleared intervals,
     near-to-far) */
  {
    auto t = MakeIv(0, a, 1000, b);
    SubtractInterval(t, MakeIv(0, a, 200, b));    /* clips start */
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 200));
    ok1(equals(t.exit.distance, 1000));
    SubtractInterval(t, MakeIv(800, a, 1100, b)); /* clips end */
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 200));
    ok1(equals(t.exit.distance, 800));
  }

  /* Case 9: already-invalid target stays invalid */
  {
    auto t = AirspaceWarningInterval::Invalid();
    SubtractInterval(t, MakeIv(100, a, 200, b));
    ok1(!t.IsValid());
  }

  /* Case 10: boundary touch (cleared.exit == target.entry) -> no-op */
  {
    auto t = MakeIv(100, a, 200, b);
    SubtractInterval(t, MakeIv(50, a, 100, b));
    ok1(t.IsValid());
    ok1(equals(t.entry.distance, 100));
    ok1(equals(t.exit.distance, 200));
  }

  return exit_status();
}
