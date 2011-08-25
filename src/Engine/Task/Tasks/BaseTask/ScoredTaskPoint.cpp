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
ScoredTaskPoint::TransitionEnter(const AircraftState & ref_now, 
                                  const AircraftState & ref_last)
{
  if (!CheckEnterTransition(ref_now, ref_last))
    return false;

  if (EntryPrecondition() && (!ScoreFirstEntry() || !HasEntered()))
    state_entered = ref_now;

  return true;
}

bool 
ScoredTaskPoint::TransitionExit(const AircraftState & ref_now, 
                                 const AircraftState &ref_last,
                                 const TaskProjection &projection)
{
  if (!CheckExitTransition(ref_now, ref_last))
    return false;

  if (ScoreLastExit()) {
    ClearSampleAllButLast(ref_last, projection);
    state_entered = ref_last;
    state_exited = ref_now;
  } else {
    state_exited = ref_last;
  }

  return true;
}


const GeoPoint &
ScoredTaskPoint::GetLocationTravelled() const
{
  return GetLocationMin();
}

const GeoPoint &
ScoredTaskPoint::GetLocationScored() const
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
  state_entered.time = fixed_minus_one;
  state_exited.time = fixed_minus_one;
}
