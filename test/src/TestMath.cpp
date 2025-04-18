// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestMath.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(N_TEST_LINE2D + N_TEST_QUADRILATERAL);

  TestLine2D();
  TestQuadrilateral();

  return exit_status();
}
