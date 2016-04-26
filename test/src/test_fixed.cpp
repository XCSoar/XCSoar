/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Math/FastMath.hpp"
#include "Math/Util.hpp"
#include "Util/Macros.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static constexpr struct {
  double input;
  int floor, ceil;
} floor_ceil_tests[] = {
  { 0, 0, 0 },

  { 1.00001, 1, 2 },
  { 1.4, 1, 2 },
  { 1.5, 1, 2 },
  { 1.99999, 1, 2 },

  { -1.00001, -2, -1 },
  { -1.4, -2, -1 },
  { -1.5, -2, -1 },
  { -1.99999, -2, -1 },
};

static void
TestFloorCeil()
{
  for (const auto &i : floor_ceil_tests) {
    ok1(floor(i.input) == i.floor);
    ok1(ceil(i.input) == i.ceil);
  }
}

static constexpr struct {
  double in;
  unsigned out;
} uround_test_values[] = {
  { 0, 0 },
  { 0.1, 0 },
  { 0.49, 0 },
  { 0.5, 1 },
  { 0.51, 1 },
  { 0.99999, 1 },
  { 1, 1 },
  { 4294967295.0, 4294967295u },
  { 4294967295.49, 4294967295u },
  { 4294967294.51, 4294967295u },
};

static constexpr struct {
  double in;
  int out;
} iround_test_values[] = {
  { 0, 0 },
  { 0.1, 0 },
  { 0.49, 0 },
  { 0.51, 1 },
  { 0.99999, 1 },
  { 1, 1 },
  { 2147483647, 2147483647 },
  { 2147483647.49, 2147483647 },
  { 2147483646.51, 2147483647 },
};

static void
TestRound()
{
  for (unsigned i = 0; i < ARRAY_SIZE(uround_test_values); ++i)
    ok1(uround(uround_test_values[i].in) == uround_test_values[i].out);

  for (unsigned i = 0; i < ARRAY_SIZE(iround_test_values); ++i) {
    ok1(iround(iround_test_values[i].in) == iround_test_values[i].out);
    ok1(iround(-iround_test_values[i].in) == -iround_test_values[i].out);
  }
}

// tolerance is 0.3%

static double Hypot_test_values[][2]={
  { 243859.6, -57083.4 },
  { -57083.4, 243859.6 },
  { 1234.0, 1234.0 },
  { -1234.0, -1234.0 },
  { 0.0, 0.0 },
};

static void test_hypot() {
  for (unsigned i = 0; i < ARRAY_SIZE(Hypot_test_values); i++) {
    double dx = Hypot_test_values[i][0];
    double dy = Hypot_test_values[i][1];
    double d = hypot(dx, dy);

    double fd(hypot(dx, dy));

    ok(fabs(fd - d) < 1.0e-3, "hypot(dx, dy)", 0);
  }
}

int main(int argc, char** argv) {
  plan_tests(2 + ARRAY_SIZE(Hypot_test_values)
             + ARRAY_SIZE(floor_ceil_tests) * 2
             + ARRAY_SIZE(uround_test_values)
             + 2 * ARRAY_SIZE(iround_test_values));

  double da = 20.0;
  double dsina = sin(da);

  double a(da);
  double sina(sin(a));

  ok(fabs(sina - dsina) < 1.0e-5, "sin(a)", 0);

  double dx = -0.3;
  double dy = 0.6;
  double dt = atan2(dy, dx);

  double x(dx);
  double y(dy);
  double t(atan2(y, x));

  ok(fabs(t - dt) < 1.0e-5, "atan(y,x)", 0);

  TestFloorCeil();
  TestRound();
  test_hypot();

  return exit_status();
}
