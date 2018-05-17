/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_UNITS_SETTINGS_HPP
#define XCSOAR_UNITS_SETTINGS_HPP

#include "Unit.hpp"
#include "Group.hpp"
#include "Compiler.h"

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

  void SetDefaults();

  /**
   * Return the configured unit for a given group.
   */
  gcc_pure
  Unit GetByGroup(UnitGroup group) const;

  bool operator==(const UnitSetting &right) const;
};

#endif
