/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
  plan_tests(92);

  // Test Native() and Native()
  ok1(equals(Angle::Native(fixed_zero).Native(), fixed_zero));
  ok1(equals(Angle::Native(fixed_one).Native(), fixed_one));
  ok1(equals(Angle::Native(fixed_two).Native(), fixed_two));
  ok1(equals(Angle::Native(fixed_90).Native(), fixed_90));

  // Test zero()
  ok1(equals(Angle::Zero().Native(), fixed_zero));

  // Test degrees()
#ifdef RADIANS
  ok1(equals(Angle::Degrees(fixed_zero).Native(), fixed_zero));
  ok1(equals(Angle::Degrees(fixed_90).Native(), fixed_half_pi));
  ok1(equals(Angle::Degrees(fixed_180).Native(), fixed_pi));
  ok1(equals(Angle::Degrees(fixed_270).Native(), fixed_pi + fixed_half_pi));
  ok1(equals(Angle::Degrees(fixed_360).Native(), fixed_two_pi));

  ok1(equals(Angle::Radians(fixed_zero).Native(), fixed_zero));
  ok1(equals(Angle::Radians(fixed_half_pi).Native(), fixed_half_pi));
  ok1(equals(Angle::Radians(fixed_pi).Native(), fixed_pi));
  ok1(equals(Angle::Radians(fixed_pi + fixed_half_pi).Native(),
                            fixed_pi + fixed_half_pi));
  ok1(equals(Angle::Radians(fixed_two_pi).Native(), fixed_two_pi));
#else
  ok1(equals(Angle::Degrees(fixed_zero).Native(), fixed_zero));
  ok1(equals(Angle::Degrees(fixed_90).Native(), fixed_90));
  ok1(equals(Angle::Degrees(fixed_180).Native(), fixed_180));
  ok1(equals(Angle::Degrees(fixed_270).Native(), fixed_270));
  ok1(equals(Angle::Degrees(fixed_360).Native(), fixed_360));

  ok1(equals(Angle::Radians(fixed_zero).Native(), fixed_zero));
  ok1(equals(Angle::Radians(fixed_half_pi).Native(), fixed_90));
  ok1(equals(Angle::Radians(fixed_pi).Native(), fixed_180));
  ok1(equals(Angle::Radians(fixed_pi + fixed_half_pi).Native(), fixed_270));
  ok1(equals(Angle::Radians(fixed_two_pi).Native(), fixed_360));
#endif

  // Test Degrees()
  ok1(equals(Angle::Degrees(fixed_90).Degrees(), 90));
  ok1(equals(Angle::Degrees(-fixed_90).Degrees(), -90));

  // Test Radians()
  ok1(equals(Angle::Degrees(fixed_90).Radians(), fixed_half_pi));
  ok1(equals(Angle::Degrees(-fixed_90).Radians(), -fixed_half_pi));

  // Test Hours()
  ok1(equals(Angle::Degrees(fixed_90).Hours(), fixed(6)));
  ok1(equals(Angle::Degrees(-fixed_90).Hours(), -fixed(6)));

  // Test DMS()
  ok1(equals(Angle::DMS(fixed_90, fixed(30), fixed(15)).Degrees(),
             fixed(90.504166667)));
  ok1(equals(Angle::DMS(fixed_90, -fixed(30), -fixed(15)).Degrees(),
             fixed(89.495833333)));
  ok1(equals(Angle::DMS(-fixed_90, -fixed(30), -fixed(15)).Degrees(),
             fixed(-90.504166667)));
  ok1(equals(Angle::DMS(-fixed_90, fixed(30), fixed(15)).Degrees(),
             fixed(-89.495833333)));

  // Test AbsoluteDegrees()
  ok1(equals(Angle::Degrees(fixed_90).AbsoluteDegrees(), 90));
  ok1(equals(Angle::Degrees(-fixed_90).AbsoluteDegrees(), 90));

  // Test AbsoluteRadians()
  ok1(equals(Angle::Degrees(fixed_90).AbsoluteRadians(), fixed_half_pi));
  ok1(equals(Angle::Degrees(-fixed_90).AbsoluteRadians(), fixed_half_pi));

  // Test Reciprocal()
  ok1(equals(Angle::Degrees(fixed_90).Reciprocal(), 270));
  ok1(equals(Angle::Degrees(fixed_270).Reciprocal(), 90));

  // Test AsBearing()
  ok1(equals(Angle::Degrees(fixed(361)).AsBearing(), 1));
  ok1(equals(Angle::Degrees(fixed(180)).AsBearing(), 180));
  ok1(equals(Angle::Degrees(fixed(-180)).AsBearing(), 180));
  ok1(equals(Angle::Degrees(fixed(-270)).AsBearing(), 90));

  // Test AsDelta()
  ok1(equals(Angle::Degrees(fixed_90).AsDelta(), 90));
  ok1(equals(Angle::Degrees(fixed(179)).AsDelta(), 179));
  ok1(equals(Angle::Degrees(fixed(-179)).AsDelta(), -179));
  ok1(equals(Angle::Degrees(fixed_270).AsDelta(), -90));

  // Test Between()
  ok1(Angle::Degrees(fixed_90).Between(Angle::Degrees(fixed_zero),
                                       Angle::Degrees(fixed_180)));

  ok1(!Angle::Degrees(fixed_90).Between(Angle::Degrees(fixed_180),
                                        Angle::Degrees(fixed_zero)));

  ok1(Angle::Degrees(fixed_zero).Between(Angle::Degrees(fixed_270),
                                         Angle::Degrees(fixed_90)));

  // Test sin()
  ok1(equals(Angle::Degrees(fixed_zero).sin(), 0));
  ok1(equals(Angle::Degrees(fixed_90).sin(), 1));
  ok1(equals(Angle::Degrees(fixed_180).sin(), 0));
  ok1(equals(Angle::Degrees(fixed_270).sin(), -1));
  ok1(equals(Angle::Degrees(fixed_360).sin(), 0));

  // Test cos()
  ok1(equals(Angle::Degrees(fixed_zero).cos(), 1));
  ok1(equals(Angle::Degrees(fixed_90).cos(), 0));
  ok1(equals(Angle::Degrees(fixed_180).cos(), -1));
  ok1(equals(Angle::Degrees(fixed_270).cos(), 0));
  ok1(equals(Angle::Degrees(fixed_360).cos(), 1));

  // Test fastsine()
  ok1(equals(Angle::Degrees(fixed_zero).fastsine(), 0));
  ok1(equals(Angle::Degrees(fixed_90).fastsine(), 1));
  ok1(equals(Angle::Degrees(fixed_180).fastsine(), 0));
  ok1(equals(Angle::Degrees(fixed_270).fastsine(), -1));
  ok1(equals(Angle::Degrees(fixed_360).fastsine(), 0));

  // Test fastcosine()
  ok1(equals(Angle::Degrees(fixed_zero).fastcosine(), 1));
  ok1(equals(Angle::Degrees(fixed_90).fastcosine(), 0));
  ok1(equals(Angle::Degrees(fixed_180).fastcosine(), -1));
  ok1(equals(Angle::Degrees(fixed_270).fastcosine(), 0));
  ok1(equals(Angle::Degrees(fixed_360).fastcosine(), 1));

  // Test BiSector()
  ok1(equals(Angle::Degrees(fixed_90).BiSector(Angle::Degrees(fixed_180)), 45));
  ok1(equals(Angle::Degrees(fixed_270).BiSector(Angle::Degrees(fixed_zero)), 225));
  ok1(equals(Angle::Degrees(fixed_270).BiSector(Angle::Degrees(fixed_180)), 315));

  // Test Fraction()
  ok1(equals(Angle::Degrees(fixed_zero).Fraction(
      Angle::Degrees(fixed_90), fixed(0.25)), 22.5));
  ok1(equals(Angle::Degrees(fixed_zero).Fraction(
      Angle::Degrees(fixed_90), fixed(0.5)), 45));
  ok1(equals(Angle::Degrees(fixed_zero).Fraction(
      Angle::Degrees(fixed_90), fixed(0.75)), 67.5));

  // Test Sign()
  ok1(Angle::Degrees(fixed_90).Sign() == 1);
  ok1(Angle::Degrees(fixed_zero).Sign() == 0);
  ok1(Angle::Degrees(-fixed_90).Sign() == -1);

  ok1(Angle::Degrees(fixed_90).Sign(fixed_one) == 1);
  ok1(Angle::Degrees(fixed_half).Sign(fixed_one) == 0);
  ok1(Angle::Degrees(fixed_zero).Sign(fixed_one) == 0);
  ok1(Angle::Degrees(-fixed_half).Sign(fixed_one) == 0);
  ok1(Angle::Degrees(-fixed_90).Sign(fixed_one) == -1);

  // Test SinCos()
  fixed sin, cos;
  Angle::Degrees(fixed(45)).SinCos(sin, cos);
  ok1(equals(sin, Angle::Degrees(fixed(45)).sin()));
  ok1(equals(cos, Angle::Degrees(fixed(45)).cos()));

  // Test Flip()
  Angle a = Angle::Degrees(fixed_90);
  a.Flip();
  ok1(equals(a.Degrees(), -fixed_90));

  // Test Flipped()
  ok1(equals(Angle::Degrees(fixed_zero).Flipped().Degrees(), fixed_zero));
  ok1(equals(Angle::Degrees(fixed_90).Flipped().Degrees(), -fixed_90));
  ok1(equals(Angle::Degrees(fixed_180).Flipped().Degrees(), -fixed_180));
  ok1(equals(Angle::Degrees(fixed_270).Flipped().Degrees(), -fixed_270));

  // Test Half()
  ok1(equals(Angle::Degrees(fixed_90).Half().Degrees(), fixed(45)));
  ok1(equals(Angle::Degrees(fixed_180).Half().Degrees(), fixed_90));

  // Test FromXY()
  ok1(equals(Angle::FromXY(fixed_one, fixed_zero), 0));
  ok1(equals(Angle::FromXY(fixed_one, fixed_one), 45));
  ok1(equals(Angle::FromXY(fixed_zero, fixed_one), 90));
  ok1(equals(Angle::FromXY(-fixed_one, fixed_one), 135));
  ok1(equals(Angle::FromXY(-fixed_one, fixed_zero), 180));
  ok1(equals(Angle::FromXY(fixed_zero, -fixed_one), -90));
  ok1(equals(Angle::FromXY(fixed_one, -fixed_one), -45));

  return exit_status();
}
