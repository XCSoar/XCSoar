/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Math/Angle.hpp"
#include "Validity.hpp"

/**
 * A container holding the aircraft current attitude state.
 */
struct AttitudeState
{
  /** Estimated bank angle */
  Angle bank_angle;

  /** Estimated pitch angle */
  Angle pitch_angle;

  /** Estimated heading */
  Angle heading;

  Validity bank_angle_available;
  Validity pitch_angle_available;
  Validity heading_available;

  /**
   * Invalidate all data held in this object.
   */
  constexpr void Reset() noexcept {
    bank_angle_available.Clear();
    pitch_angle_available.Clear();
    heading_available.Clear();
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const AttitudeState &add) noexcept;

  void Expire(TimeStamp now) noexcept;
};
