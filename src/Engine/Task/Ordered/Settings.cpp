// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
OrderedTaskSettings::SetDefaults()
{
  aat_min_time = std::chrono::hours{3};
  start_constraints.SetDefaults();
  finish_constraints.SetDefaults();
  fai_triangle.SetDefaults();
}
