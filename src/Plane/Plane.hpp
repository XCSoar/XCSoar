// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "Polar/Shape.hpp"

struct Plane
{
  StaticString<32> registration;
  StaticString<6> competition_id;
  StaticString<32> type;

  StaticString<32> polar_name;

  PolarShape polar_shape;

  double empty_mass;
  double dry_mass_obsolete; // unused entry for plane file compatibility. to be removed 2023..
  double max_ballast;
  double max_speed;
  double wing_area;

  /** Time to drain full ballast (s) */
  unsigned dump_time;

  unsigned handicap;

  /**
   * Type of glider from a list, published by WeGlide server to select
   * the correct glider id for the flight to upload.  The list is
   * published on https://raw.githubusercontent.com/ the data of the
   * selected glider you can find on
   * https://api.weglide.org/v1/aircraft/$(ID)
   */
  unsigned weglide_glider_type;
};
