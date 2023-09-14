// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskCruiseEfficiency.hpp"

double
TaskCruiseEfficiency::f(const double ce) noexcept
{
  tm.set_cruise_efficiency(ce);
  return time_error();
}
