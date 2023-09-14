// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Unit.hpp"
#include "Group.hpp"

#include <compare> // for the defaulted spaceship operator

struct UnitSetting
{
  /** Unit for distances */
  Unit distance_unit;
  /** Unit for altitudes, heights */
  Unit altitude_unit;
  /** Unit for temperature */
  Unit temperature_unit;
  /** Unit for aircraft speeds */
  Unit speed_unit;
  /** Unit for vertical speeds, varios */
  Unit vertical_speed_unit;
  /** Unit for wind speeds */
  Unit wind_speed_unit;
  /** Unit for task speeds */
  Unit task_speed_unit;
  /** Unit for pressures */
  Unit pressure_unit;
  /** Unit for wing loading */
  Unit wing_loading_unit;
  /** Unit for mass */
  Unit mass_unit;
  /** Unit for rotation speed */
  Unit rotation_unit;

  void SetDefaults() noexcept;

  /**
   * Return the configured unit for a given group.
   */
  [[gnu::pure]]
  Unit GetByGroup(UnitGroup group) const noexcept;

  friend constexpr auto operator<=>(const UnitSetting &,
                                    const UnitSetting &) noexcept = default;
};
