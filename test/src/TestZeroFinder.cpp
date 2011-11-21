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

#include "Engine/Util/ZeroFinder.hpp"
#include "TestUtil.hpp"

class ZeroFinderTest: public ZeroFinder
{
  unsigned func;

public:
  ZeroFinderTest(const fixed &x_min, const fixed &x_max, unsigned _func = 0) :
    ZeroFinder(x_min, x_max, fixed(0.0001)), func(_func) {}

  fixed f(const fixed x);
};

fixed
ZeroFinderTest::f(const fixed x)
{
  if (func == 0)
    return fixed(2) * x * x - fixed(3) * x - fixed(5);

  if (func == 1)
    return pow(fixed(2), x) - fixed(3);

  if (func == 2)
    return cos(x);

  assert(true);
  return fixed_zero;
}

int main(int argc, char **argv)
{
  plan_tests(18);

  ZeroFinderTest zf(fixed(-100), fixed(100), 0);
  ok1(equals(zf.find_zero(fixed(-150)), fixed(-1)));
  ok1(equals(zf.find_zero(fixed(0)), fixed(-1)));
  // ok1(equals(zf.find_zero(fixed(140)), fixed(2.5))); ???
  ok1(equals(zf.find_zero(fixed(140)), fixed(-1)));

  ok1(equals(zf.find_min(fixed(-150)), fixed(0.75)));
  ok1(equals(zf.find_min(fixed(0)), fixed(0.75)));
  ok1(equals(zf.find_min(fixed(150)), fixed(0.75)));

  ZeroFinderTest zf2(fixed(0), fixed(100), 0);
  ok1(equals(zf2.find_zero(fixed(-150)), fixed(2.5)));
  ok1(equals(zf2.find_zero(fixed(0)), fixed(2.5)));
  ok1(equals(zf2.find_zero(fixed(140)), fixed(2.5)));

  ZeroFinderTest zf3(fixed(0), fixed(10), 1);
  ok1(equals(zf3.find_zero(fixed(-150)), fixed(1.584963)));
  ok1(equals(zf3.find_zero(fixed(1)), fixed(1.584963)));
  ok1(equals(zf3.find_zero(fixed(140)), fixed(1.584963)));

  ZeroFinderTest zf4(fixed(0), fixed_pi + fixed_one, 2);
  ok1(equals(zf4.find_zero(fixed(-150)), fixed_half_pi));
  ok1(equals(zf4.find_zero(fixed(1)), fixed_half_pi));
  ok1(equals(zf4.find_zero(fixed(140)), fixed_half_pi));

  ok1(equals(zf4.find_min(fixed(-150)), fixed_pi));
  ok1(equals(zf4.find_min(fixed(1)), fixed_pi));
  ok1(equals(zf4.find_min(fixed(140)), fixed_pi));

  return exit_status();
}
