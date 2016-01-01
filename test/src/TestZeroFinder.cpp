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

#include "Math/ZeroFinder.hpp"
#include "TestUtil.hpp"

class ZeroFinderTest: public ZeroFinder
{
  unsigned func;

public:
  ZeroFinderTest(double x_min, double x_max, unsigned _func = 0) :
    ZeroFinder(x_min, x_max, 0.0001), func(_func) {}

  double f(const double x);
};

double
ZeroFinderTest::f(const double x)
{
  if (func == 0)
    return 2 * x * x - 3 * x - 5;

  if (func == 1)
    return pow(2, x) - 3;

  if (func == 2)
    return cos(x);

  gcc_unreachable();
  assert(false);
  return 0;
}

int main(int argc, char **argv)
{
  plan_tests(18);

  ZeroFinderTest zf(-100, 100, 0);
  ok1(equals(zf.find_zero(-150), -1));
  ok1(equals(zf.find_zero(0), -1));
  // ok1(equals(zf.find_zero(140), 2.5)); ???
  ok1(equals(zf.find_zero(140), -1));

  ok1(equals(zf.find_min(-150), 0.75));
  ok1(equals(zf.find_min(0), 0.75));
  ok1(equals(zf.find_min(150), 0.75));

  ZeroFinderTest zf2(0, 100, 0);
  ok1(equals(zf2.find_zero(-150), 2.5));
  ok1(equals(zf2.find_zero(0), 2.5));
  ok1(equals(zf2.find_zero(140), 2.5));

  ZeroFinderTest zf3(0, 10, 1);
  ok1(equals(zf3.find_zero(-150), 1.584963));
  ok1(equals(zf3.find_zero(1), 1.584963));
  ok1(equals(zf3.find_zero(140), 1.584963));

  ZeroFinderTest zf4(0, M_PI + 1, 2);
  ok1(equals(zf4.find_zero(-150), M_PI_2));
  ok1(equals(zf4.find_zero(1), M_PI_2));
  ok1(equals(zf4.find_zero(140), M_PI_2));

  ok1(equals(zf4.find_min(-150), M_PI));
  ok1(equals(zf4.find_min(1), M_PI));
  ok1(equals(zf4.find_min(140), M_PI));

  return exit_status();
}
