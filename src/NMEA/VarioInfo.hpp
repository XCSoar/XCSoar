// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LiftDatabase.hpp"

#include <type_traits>

/** Derived vario data */
struct VarioInfo
{
  double sink_rate;

  /** Average vertical speed based on 30s */
  double average;
  /** Average vertical speed of the airmass based on 30s */
  double netto_average;

  /** Instant glide ratio over ground */
  double gr;
  /** Glide ratio over ground while in Cruise mode */
  double cruise_gr;

  /**
   * Average glide ratio over ground.  Zero means the value is not available.
   */
  double average_gr;

  /** Instant lift/drag ratio */
  double ld_vario;

  /**
   * The lift of each ten degrees while circling.
   * Index 1 equals 5 to 15 degrees.
   */
  LiftDatabase lift_database;

  void Clear();
};

static_assert(std::is_trivial<VarioInfo>::value, "type is not trivial");
