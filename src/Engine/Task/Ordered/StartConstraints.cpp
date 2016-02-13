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

#include "StartConstraints.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Navigation/Aircraft.hpp"

void
StartConstraints::SetDefaults()
{
  open_time_span = RoughTimeSpan::Invalid();
  max_speed = 0;
  max_height = 0;
  max_height_ref = AltitudeReference::AGL;
  require_arm = false;
  fai_finish = false;
}

bool
StartConstraints::CheckSpeed(double ground_speed,
                             const TaskStartMargins *margins) const
{
  if (max_speed == 0)
    return true;

  if (fai_finish)
    return true;

  const auto margin = margins != nullptr
    ? margins->max_speed_margin
    : 0;

  return ground_speed <= max_speed + margin;
}

bool
StartConstraints::CheckHeight(const AircraftState &state,
                              const double start_elevation,
                              const TaskStartMargins *margins) const
{
  if (max_height == 0)
    return true;

  if (fai_finish)
    return true;

  const unsigned margin = margins != nullptr
    ? margins->max_height_margin
    : 0u;

  if (max_height_ref == AltitudeReference::MSL)
    return state.altitude <= max_height + margin;
  else
    return state.altitude <= max_height + margin + start_elevation;
}
