/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "ScoredTaskPoint.hpp"

ScoredTaskPoint::ScoredTaskPoint(Type _type,
                                 const Waypoint & wp, 
                                 const bool b_scored): 
  SampledTaskPoint(_type, wp, b_scored)
{
  Reset();
}

bool 
ScoredTaskPoint::transition_enter(const AircraftState & ref_now, 
                                  const AircraftState & ref_last)
{
  bool entered = CheckEnterTransition(ref_now, ref_last);
  if (entered && entry_precondition()) {
    if (!score_first_entry() || !HasEntered()) {
      m_state_entered = ref_now;
      return true;
    }
  }
  return entered;
}

bool 
ScoredTaskPoint::transition_exit(const AircraftState & ref_now, 
                                 const AircraftState &ref_last,
                                 const TaskProjection &projection)
{
  bool exited = CheckExitTransition(ref_now, ref_last);
  if (exited) {
    if (score_last_exit()) {
      ClearSampleAllButLast(ref_last, projection);
      m_state_entered = ref_last;
      m_state_exited = ref_now;
     } else {
      m_state_exited = ref_last;
    }
  }
  return exited;
}


const GeoPoint &
ScoredTaskPoint::get_location_travelled() const
{
  return GetLocationMin();
}

const GeoPoint &
ScoredTaskPoint::get_location_scored() const
{
  if (boundary_scored || !HasEntered()) {
    return GetLocationMin();
  } else {
    return GetLocation();
  }
}

const GeoPoint &
ScoredTaskPoint::GetLocationRemaining() const
{
  return GetLocationMin();
}

void 
ScoredTaskPoint::Reset()
{
  SampledTaskPoint::Reset();
  m_state_entered.time = fixed_minus_one;
  m_state_exited.time = fixed_minus_one;
}
