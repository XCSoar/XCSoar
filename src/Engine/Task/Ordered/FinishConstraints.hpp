/* Copyright_License {

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

#ifndef XCSOAR_FINISH_CONSTRAINTS_HPP
#define XCSOAR_FINISH_CONSTRAINTS_HPP

#include "Geo/AltitudeReference.hpp"
#include "Compiler.h"

struct AircraftState;

struct FinishConstraints {
  /** Minimum height AGL (m) allowed to finish */
  unsigned min_height;

  /** Reference for min finish height */
  AltitudeReference min_height_ref;

  /**
   * Whether ordered task start and finish requires FAI height rules
   * and (no) speed rule.  The default value is
   * TaskFactoryConstraints::fai_finish.
   *
   * When you modify this value, remember to always keep it in sync
   * with StartConstraints::fai_finish!
   */
  bool fai_finish;

  void SetDefaults();

  /**
   * Check whether aircraft height is within finish height limit
   *
   * @param state Aircraft state
   * @param finish_elevation finish point elevation
   *
   * @return True if within limits
   */
  gcc_pure
  bool CheckHeight(const AircraftState &state,
                   double finish_elevation) const;
};

#endif
