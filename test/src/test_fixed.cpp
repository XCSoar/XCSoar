/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Math/fixed.hpp"
#include "Math/FastMath.h"
#include "Util/Macros.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

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
    ok1(uround(fixed(uround_test_values[i].in)) == uround_test_values[i].out);

  for (unsigned i = 0; i < ARRAY_SIZE(iround_test_values); ++i) {
    ok1(iround(fixed(iround_test_values[i].in)) == iround_test_values[i].out);
    ok1(iround(fixed(-iround_test_values[i].in)) == -iround_test_values[i].out);
  }
}

// tolerance is 0.3%

static void test_mag_rmag(double mag) {
  fixed px(7.07106*mag);
  fixed py(-7.07106*mag);

  double msq(px * px + py * py);
  printf("# testing mag i %g %g\n", mag, msq);

  fixed d; fixed inv_d;
  mag_rmag(px, py, d, inv_d);
  fixed ed = fabs(d-fixed(10.0*mag))/fixed(0.03*mag);
  if (ed>= fixed(1)) {
    printf("# d %g %g %g\n", (double)d, (double)ed, mag);
  }
  ok(ed< fixed(1), "mag_rmag d", 0);
  fixed inv_ed = fabs(inv_d-fixed(0.1/mag))/fixed(3.0e-4/mag);
  if (inv_ed>= fixed(1)) {
    printf("# inv %g %g %g\n", (double)inv_d, (double)inv_ed, mag);
  }
  ok(inv_ed< fixed(1), "mag_rmag inv_d", 0);
}

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

    fixed fdx(dx);
    fixed fdy(dy);
    fixed fd(MediumHypot(fdx, fdy));

    ok(fabs(fd - fixed(d)) < fixed(1.0e-3), "hypot(dx, dy)", 0);
  }
}

static constexpr double tiny_hypot_test_values[][2] = {
  { 3, 3 },
  { 2, 2 },
  { 1.5, 1.5 },
  { 1, 1 },
  { 0.1, 0.1 },
  { 0.01, 0.01 },
  { 0.001, 0.001 },
  { 0.0001, 0.0001 },
  { 0.00001, 0.00001 },
  { 0.000001, 0.000001 },
  { 0.0, 0.0 },
};

static void
TestTinyHypot()
{
  for (unsigned i = 0; i < ARRAY_SIZE(tiny_hypot_test_values); i++) {
    double dx = tiny_hypot_test_values[i][0];
    double dy = tiny_hypot_test_values[i][1];
    double d = hypot(dx, dy);

    fixed fdx(dx);
    fixed fdy(dy);
    fixed fd(TinyHypot(fdx, fdy));

    ok(fabs(fd - fixed(d)) < fixed(1.0e-3), "hypot(dx, dy)", 0);
  }
}

int main(int argc, char** argv) {
  plan_tests(43 + ARRAY_SIZE(Hypot_test_values)
             + ARRAY_SIZE(uround_test_values)
             + 2 * ARRAY_SIZE(iround_test_values)
             + ARRAY_SIZE(tiny_hypot_test_values));

  /* check the division operator */
  ok((fixed(1) / fixed(1)) * fixed(1000) == fixed(1000), "1/1", 0);
  ok((fixed(2) / fixed(2)) * fixed(1000) == fixed(1000), "2/2", 0);
  ok((fixed(1) / fixed(2)) * fixed(1000) == fixed(500), "1/2", 0);
  ok((fixed(1000) / fixed(100)) * fixed(1000) == fixed(10000), "1000/100", 0);
  ok((fixed(100) / fixed(20)) * fixed(1000) == fixed(5000), "100/20", 0);
  ok((fixed(1000000) / fixed(2)) * fixed(1000) == fixed(500000000), "1M/2", 0);
  ok((fixed(-1) / fixed(1)) * fixed(1000) == -fixed(1000), "-1/1", 0);
  ok((fixed(1) / fixed(-1)) * fixed(1000) == -fixed(1000), "1/-1", 0);
  ok((fixed(-1) / fixed(-1)) * fixed(1000) == fixed(1000), "-1/-1", 0);
  ok((fixed(-1000000) / fixed(2)) * fixed(1000) == -fixed(500000000), "-1M/2", 0);
  ok((long)((fixed(1) / (fixed(1) / fixed(10))) * fixed(1000)) == (10000), "1/0.1", 0);
  ok((long)((fixed(1) / (fixed(1) / fixed(-10))) * fixed(1000)) == -(10000) ||
     (long)((fixed(1) / (fixed(1) / fixed(-10))) * fixed(1000)) == -(10001), "1/-0.1", 0);

  ok(equals(fixed(1) / fixed(0.5), 2), "1/0.5", 0);
  ok(equals(fixed(1000) / fixed(0.5), 2000), "1/0.5", 0);
  ok(equals(fixed(1000) / (fixed(1) / 5), 5000), "1/0.5", 0);

  ok(equals(fixed(1000000) / (fixed(1) / 5), 5000000), "1/0.5", 0);
  ok(equals(fixed(10000000) / (fixed(1) / 5), 50000000), "1/0.5", 0);

  double da = 20.0;
  double dsina = sin(da);

  fixed a(da);
  fixed sina(sin(a));

  printf("a=%g, sin(a)=%g\n", FIXED_DOUBLE(a), FIXED_DOUBLE(sina));
  printf("a=%g, sin(a)=%g\n", da, dsina);

  ok(fabs(sina - fixed(dsina)) < fixed(1.0e-5), "sin(a)", 0);

  double dx = -0.3;
  double dy = 0.6;
  double dt = atan2(dy, dx);

  fixed x(dx);
  fixed y(dy);
  fixed t(atan2(y, x));

  printf("x=%g, y=%g atan(y,x)=%g\n",
         FIXED_DOUBLE(x), FIXED_DOUBLE(y), FIXED_DOUBLE(t));
  printf("x=%g, y=%g atan(y,x)=%g\n", dx, dy, dt);

  ok(fabs(t - fixed(dt)) < fixed(1.0e-5), "atan(y,x)", 0);

  {
    for (int i=1; i<=2048; i*= 2) {
      test_mag_rmag(i);
    }
  }

  TestRound();
  test_hypot();
  TestTinyHypot();

  return exit_status();
}
