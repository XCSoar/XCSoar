// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Rough/RoughSpeed.hpp"
#include "TestUtil.hpp"

#include <limits>

int main()
{
  plan_tests(13);

  ok1(double(RoughSpeed(0)) == 0);
  ok1(double(RoughSpeed(-10)) == 0);
  ok1(double(RoughSpeed(std::numeric_limits<double>::quiet_NaN())) == 0);

  /* Typical FLARM PFLAA values and the IsPassive() 4 m/s threshold. */
  ok1(equals(double(RoughSpeed(4)), 4));
  ok1(equals(double(RoughSpeed(24)), 24));
  ok1(equals(double(RoughSpeed(32)), 32));
  ok1(equals(double(RoughSpeed(127)), 127));

  /* 128 m/s overflowed the old 1/512 scale (uint16 max / 512 ≈ 128). */
  ok1(equals(double(RoughSpeed(128)), 128));
  ok1(between(double(RoughSpeed(223.3)), 223.2, 223.4));

  ok1(equals(double(RoughSpeed(1023)), 1023));
  ok1(equals(double(RoughSpeed(2000)), 1023));

  ok1(RoughSpeed(3) < 4);
  ok1(!(RoughSpeed(4) < 4));

  return exit_status();
}
