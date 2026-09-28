// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/SpeedVector.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(8);

  /* tracking due north at 30 m/s */
  constexpr SpeedVector north{Angle::Zero(), 30};

  /* a target straight ahead is approached at the whole ground speed */
  ok1(equals(north.ComponentAlong(Angle::Zero()), 30));

  /* a target abeam is not approached at all */
  ok1(is_zero(north.ComponentAlong(Angle::QuarterCircle())));

  /* a target behind recedes at the whole ground speed */
  ok1(equals(north.ComponentAlong(Angle::HalfCircle()), -30));

  /* 60 degrees off track leaves half of the ground speed */
  ok1(equals(north.ComponentAlong(Angle::Degrees(60)), 15));

  /* only the magnitude of the angle matters, not the side the target
     is on */
  ok1(equals(north.ComponentAlong(Angle::Degrees(-60)), 15));

  /* the bearings need no normalisation beforehand */
  ok1(is_zero(north.ComponentAlong(Angle::Degrees(270))));

  /* more than 90 degrees off track means the distance grows */
  ok1(north.ComponentAlong(Angle::Degrees(120)) < 0);

  /* standing still makes no distance good in any direction */
  ok1(is_zero(SpeedVector::Zero().ComponentAlong(Angle::Degrees(42))));

  return exit_status();
}
