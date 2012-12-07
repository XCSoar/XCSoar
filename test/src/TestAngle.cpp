/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Math/Angle.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  plan_tests(105);

  // Test Native() and Native()
  ok1(equals(Angle::Native(fixed(0)).Native(), fixed(0)));
  ok1(equals(Angle::Native(fixed(1)).Native(), fixed(1)));
  ok1(equals(Angle::Native(fixed(2)).Native(), fixed(2)));
  ok1(equals(Angle::Native(fixed(90)).Native(), fixed(90)));

  // Test zero()
  ok1(equals(Angle::Zero().Native(), fixed(0)));

  // test constants
  ok1(equals(Angle::QuarterCircle().Radians(), fixed_half_pi));
  ok1(equals(Angle::HalfCircle().Radians(), fixed_pi));
  ok1(equals((Angle::HalfCircle() + Angle::QuarterCircle()).Radians(),
             fixed_pi + fixed_half_pi));
  ok1(equals(Angle::FullCircle().Radians(), fixed_two_pi));

  // Test degrees()
#ifdef RADIANS
  ok1(equals(Angle::Degrees(0).Native(), fixed(0)));
  ok1(equals(Angle::Degrees(90).Native(), fixed_half_pi));
  ok1(equals(Angle::Degrees(180).Native(), fixed_pi));
  ok1(equals(Angle::Degrees(270).Native(), fixed_pi + fixed_half_pi));
  ok1(equals(Angle::Degrees(360).Native(), fixed_two_pi));

  ok1(equals(Angle::Radians(fixed(0)).Native(), fixed(0)));
  ok1(equals(Angle::Radians(fixed_half_pi).Native(), fixed_half_pi));
  ok1(equals(Angle::Radians(fixed_pi).Native(), fixed_pi));
  ok1(equals(Angle::Radians(fixed_pi + fixed_half_pi).Native(),
                            fixed_pi + fixed_half_pi));
  ok1(equals(Angle::Radians(fixed_two_pi).Native(), fixed_two_pi));
#else
  ok1(equals(Angle::Degrees(0).Native(), fixed(0)));
  ok1(equals(Angle::Degrees(90).Native(), fixed(90)));
  ok1(equals(Angle::Degrees(180).Native(), fixed(180)));
  ok1(equals(Angle::Degrees(270).Native(), fixed(270)));
  ok1(equals(Angle::Degrees(360).Native(), fixed(360)));

  ok1(equals(Angle::Radians(fixed(0)).Native(), fixed(0)));
  ok1(equals(Angle::Radians(fixed_half_pi).Native(), fixed(90)));
  ok1(equals(Angle::Radians(fixed_pi).Native(), fixed(180)));
  ok1(equals(Angle::Radians(fixed_pi + fixed_half_pi).Native(), fixed(270)));
  ok1(equals(Angle::Radians(fixed_two_pi).Native(), fixed(360)));
#endif

  // Test Degrees()
  ok1(equals(Angle::Degrees(90).Degrees(), 90));
  ok1(equals(Angle::Degrees(-90).Degrees(), -90));

  // Test Radians()
  ok1(equals(Angle::Degrees(90).Radians(), fixed_half_pi));
  ok1(equals(Angle::Degrees(-90).Radians(), -fixed_half_pi));

  // Test Hours()
  ok1(equals(Angle::Degrees(90).Hours(), fixed(6)));
  ok1(equals(Angle::Degrees(-90).Hours(), -fixed(6)));

  // Test DMS()
  ok1(equals(Angle::DMS(90, 30, 15).Degrees(),
             fixed(90.504166667)));

  // Test AbsoluteDegrees()
  ok1(equals(Angle::Degrees(90).AbsoluteDegrees(), 90));
  ok1(equals(Angle::Degrees(-90).AbsoluteDegrees(), 90));

  // Test AbsoluteRadians()
  ok1(equals(Angle::Degrees(90).AbsoluteRadians(), fixed_half_pi));
  ok1(equals(Angle::Degrees(-90).AbsoluteRadians(), fixed_half_pi));

  // Test Reciprocal()
  ok1(equals(Angle::Degrees(90).Reciprocal(), 270));
  ok1(equals(Angle::Degrees(270).Reciprocal(), 90));

  // Test AsBearing()
  ok1(equals(Angle::Degrees(361).AsBearing(), 1));
  ok1(equals(Angle::Degrees(180).AsBearing(), 180));
  ok1(equals(Angle::Degrees(-180).AsBearing(), 180));
  ok1(equals(Angle::Degrees(-270).AsBearing(), 90));

  // Test AsDelta()
  ok1(equals(Angle::Degrees(90).AsDelta(), 90));
  ok1(equals(Angle::Degrees(179).AsDelta(), 179));
  ok1(equals(Angle::Degrees(-179).AsDelta(), -179));
  ok1(equals(Angle::Degrees(270).AsDelta(), -90));

  // Test Between()
  ok1(Angle::QuarterCircle().Between(Angle::Zero(), Angle::HalfCircle()));
  ok1(!Angle::QuarterCircle().Between(Angle::HalfCircle(), Angle::Zero()));
  ok1(Angle::Zero().Between(Angle::Degrees(270), Angle::Degrees(90)));

  // Test sin()
  ok1(equals(Angle::Zero().sin(), 0));
  ok1(equals(Angle::Degrees(90).sin(), 1));
  ok1(equals(Angle::Degrees(180).sin(), 0));
  ok1(equals(Angle::Degrees(270).sin(), -1));
  ok1(equals(Angle::Degrees(360).sin(), 0));

  // Test cos()
  ok1(equals(Angle::Zero().cos(), 1));
  ok1(equals(Angle::Degrees(90).cos(), 0));
  ok1(equals(Angle::Degrees(180).cos(), -1));
  ok1(equals(Angle::Degrees(270).cos(), 0));
  ok1(equals(Angle::Degrees(360).cos(), 1));

  // Test fastsine()
  ok1(equals(Angle::Zero().fastsine(), 0));
  ok1(equals(Angle::Degrees(90).fastsine(), 1));
  ok1(equals(Angle::Degrees(180).fastsine(), 0));
  ok1(equals(Angle::Degrees(270).fastsine(), -1));
  ok1(equals(Angle::Degrees(360).fastsine(), 0));

  // Test fastcosine()
  ok1(equals(Angle::Zero().fastcosine(), 1));
  ok1(equals(Angle::Degrees(90).fastcosine(), 0));
  ok1(equals(Angle::Degrees(180).fastcosine(), -1));
  ok1(equals(Angle::Degrees(270).fastcosine(), 0));
  ok1(equals(Angle::Degrees(360).fastcosine(), 1));

  // Test BiSector()
  ok1(equals(Angle::Degrees(270).HalfAngle(Angle::Degrees(180)), 45));
  ok1(equals(Angle::Degrees(90).HalfAngle(Angle::Zero()), 225));
  ok1(equals(Angle::Degrees(90).HalfAngle(Angle::Degrees(180)), 315));

  // Test Fraction()
  ok1(equals(Angle::Zero().Fraction(Angle::Degrees(90), fixed(0.25)), 22.5));
  ok1(equals(Angle::Zero().Fraction(Angle::Degrees(90), fixed(0.5)), 45));
  ok1(equals(Angle::Zero().Fraction(Angle::Degrees(90), fixed(0.75)), 67.5));

  // Test Sign()
  ok1(Angle::Degrees(90).Sign() == 1);
  ok1(Angle::Zero().Sign() == 0);
  ok1(Angle::Degrees(-90).Sign() == -1);

  ok1(Angle::Degrees(90).Sign(fixed(1)) == 1);
  ok1(Angle::Degrees(fixed(0.5)).Sign(fixed(1)) == 0);
  ok1(Angle::Zero().Sign(fixed(1)) == 0);
  ok1(Angle::Degrees(-fixed(0.5)).Sign(fixed(1)) == 0);
  ok1(Angle::Degrees(-90).Sign(fixed(1)) == -1);

  // Test SinCos()
  const auto sc = Angle::Degrees(45).SinCos();
  fixed sin = sc.first, cos = sc.second;
  ok1(equals(sin, Angle::Degrees(45).sin()));
  ok1(equals(cos, Angle::Degrees(45).cos()));

  // Test Flip()
  Angle a = Angle::Degrees(90);
  a.Flip();
  ok1(equals(a.Degrees(), -fixed(90)));

  // Test Flipped()
  ok1(equals(Angle::Zero().Flipped().Degrees(), fixed(0)));
  ok1(equals(Angle::Degrees(90).Flipped().Degrees(), -fixed(90)));
  ok1(equals(Angle::Degrees(180).Flipped().Degrees(), -fixed(180)));
  ok1(equals(Angle::Degrees(270).Flipped().Degrees(), -fixed(270)));

  // Test Half()
  ok1(equals(Angle::Degrees(90).Half().Degrees(), fixed(45)));
  ok1(equals(Angle::Degrees(180).Half().Degrees(), fixed(90)));

  // Test FromXY()
  ok1(equals(Angle::FromXY(fixed(1), fixed(0)), 0));
  ok1(equals(Angle::FromXY(fixed(1), fixed(1)), 45));
  ok1(equals(Angle::FromXY(fixed(0), fixed(1)), 90));
  ok1(equals(Angle::FromXY(fixed(-1), fixed(1)), 135));
  ok1(equals(Angle::FromXY(fixed(-1), fixed(0)), 180));
  ok1(equals(Angle::FromXY(fixed(0), fixed(-1)), -90));
  ok1(equals(Angle::FromXY(fixed(1), fixed(-1)), -45));

  // Test CompareRoughly()
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(0)));
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(5)));
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(9.9)));
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(10)));
  ok1(!Angle::Degrees(0).CompareRoughly(Angle::Degrees(10.1)));
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(-9.9)));
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(-10)));
  ok1(!Angle::Degrees(0).CompareRoughly(Angle::Degrees(-10.1)));

  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(4), Angle::Degrees(5)));
  ok1(Angle::Degrees(0).CompareRoughly(Angle::Degrees(-4), Angle::Degrees(5)));
  ok1(!Angle::Degrees(0).CompareRoughly(Angle::Degrees(6), Angle::Degrees(5)));
  ok1(!Angle::Degrees(0).CompareRoughly(Angle::Degrees(-6), Angle::Degrees(5)));

  return exit_status();
}
