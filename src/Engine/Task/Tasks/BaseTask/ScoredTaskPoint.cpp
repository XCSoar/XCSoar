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

ScoredTaskPoint::ScoredTaskPoint(enum type _type,
                                 const Waypoint & wp, 
                                 const bool b_scored): 
  SampledTaskPoint(_type, wp, b_scored)
{
  reset();
}

bool 
ScoredTaskPoint::transition_enter(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool entered = check_transition_enter(ref_now, ref_last);
  if (entered && entry_precondition()) {
    if (!score_first_entry() || !has_entered()) {
      m_state_entered = ref_now;
      return true;
    }
  }
  return entered;
}

bool 
ScoredTaskPoint::transition_exit(const AIRCRAFT_STATE & ref_now, 
                                 const AIRCRAFT_STATE &ref_last,
                                 const TaskProjection &projection)
{
  bool exited = check_transition_exit(ref_now, ref_last);
  if (exited) {
    if (score_last_exit()) {
      clear_sample_all_but_last(ref_last, projection);
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
  return get_location_min();
}

const GeoPoint &
ScoredTaskPoint::get_location_scored() const
{
  if (m_boundary_scored || !has_entered()) {
    return get_location_min();
  } else {
    return get_location();
  }
}

const GeoPoint &
ScoredTaskPoint::get_location_remaining() const
{
  return get_location_min();
}

void 
ScoredTaskPoint::reset()
{
  SampledTaskPoint::reset();
  m_state_entered.Time = fixed_minus_one;
  m_state_exited.Time = fixed_minus_one;
}
