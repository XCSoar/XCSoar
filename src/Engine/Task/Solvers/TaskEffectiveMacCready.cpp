// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskEffectiveMacCready.hpp"

double
TaskEffectiveMacCready::f(const double mc) noexcept
{
  tm.set_mc(mc);
  return time_error();
}
