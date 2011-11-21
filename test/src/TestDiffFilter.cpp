/* Copyright_License {

 XCSoar Glide Computer - http://www.xcsoar.org/
 Copyright (C) 2000-2011 The XCSoar Project
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

#include "Engine/Util/DiffFilter.hpp"

#include "TestUtil.hpp"

#include <cstdio>

int
main(int argc, char **argv)
{
  plan_tests(192 + 232 + 40);

  DiffFilter df;

  // Test steady-state response scaling
  for (long dY = 1; dY <= 10000000; dY *= 10) {
    df.reset();
    for (int Y = dY; Y < 30 * dY; Y += dY) {
      if (Y < 6 * dY)
        // Give the filter time to calm down before testing for steady state
        df.update(fixed(Y));
      else {
        // test if the filter response is close enough to dX
        //
        // the discrete filter design results in steady-state error
        // of approximately 3.4507 percent
        fixed error = fabs((df.update(fixed(Y)) - fixed(dY)) / dY);
        ok1(error < fixed(0.035));
      }
    }
  }

  // Test steady-state response scaling with reset(0, dX) call
  for (long dY = 1; dY <= 10000000; dY *= 10) {
    df.reset(fixed_zero, fixed(dY));
    for (int Y = dY; Y < 30 * dY; Y += dY) {
      // test if the filter response is close enough to dX
      //
      // the discrete filter design results in steady-state error
      // of approximately 3.4507 percent
      fixed error = fabs((df.update(fixed(Y)) - fixed(dY)) / dY);
      ok1(error < fixed(0.035));
    }
  }

  df.reset();
  fixed p = fixed_two_pi / fixed(10);
  for (int X = 0; X < 50; X += 1) {
    // Y = sin(2 * pi * (X/10)
    fixed Y = fixed(sin(p * X));

    fixed dY_shifted = fixed(cos(p * (X - 3))) * p;
    fixed dY_filter = df.update(fixed(Y));
    fixed error = fabs(dY_filter - dY_shifted);
    if (X >= 10)
      ok1(error < fixed(0.05));
  }

  return exit_status();
}
