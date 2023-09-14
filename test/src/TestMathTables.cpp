// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math/FastMath.hpp"
#include "Computer/ThermalRecency.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(THERMALRECENCY_SIZE);

  for (unsigned i = 0; i < THERMALRECENCY_SIZE; ++i)
    ok1((int)(thermal_fn(i) * 1024 * 1024) ==
        (int)(thermal_recency_fn(i) * 1024 * 1024));

  return exit_status();
}
