// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math/ZeroFinder.hpp"
#include "util/Compiler.h"
#include "TestUtil.hpp"

class ZeroFinderTest: public ZeroFinder
{
  unsigned func;

public:
  ZeroFinderTest(double x_min, double x_max, unsigned _func = 0) :
    ZeroFinder(x_min, x_max, 0.0001), func(_func) {}

  double f(const double x) noexcept override;
};

double
ZeroFinderTest::f(const double x) noexcept
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

int main()
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
