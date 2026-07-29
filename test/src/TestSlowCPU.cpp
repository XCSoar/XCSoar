// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Hardware/CPU.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(5);

  ok1(!IsSlowCPUFrequency(0));
  ok1(IsSlowCPUFrequency(1000000));
  ok1(IsSlowCPUFrequency(SLOW_CPU_MAX_FREQ_KHZ));
  ok1(!IsSlowCPUFrequency(SLOW_CPU_MAX_FREQ_KHZ + 1));
  ok1(!IsSlowCPUFrequency(1500000));

  return exit_status();
}
