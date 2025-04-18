// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct VarioSettings {
  bool show_average;
  bool show_mc;
  bool show_speed_to_fly;
  bool show_ballast;
  bool show_bugs;
  bool show_gross;
  bool show_average_needle;
  bool show_thermal_average_needle;

  void SetDefaults() noexcept;
};
