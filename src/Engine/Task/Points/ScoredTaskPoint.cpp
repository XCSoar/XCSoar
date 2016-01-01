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
#include "ScoredTaskPoint.hpp"

ScoredTaskPoint::ScoredTaskPoint(const GeoPoint &location, bool b_scored)
  :SampledTaskPoint(location, b_scored)
{
  Reset();
}

bool 
ScoredTaskPoint::TransitionEnter(const AircraftState &ref_now,
                                 const AircraftState &ref_last)
{
  if (!CheckEnterTransition(ref_now, ref_last))
    return false;

  if (EntryPrecondition() && (!ScoreFirstEntry() || !HasEntered()))
    state_entered = ref_now;

  return true;
}

bool 
ScoredTaskPoint::TransitionExit(const AircraftState &ref_now,
                                const AircraftState &ref_last,
                                const FlatProjection &projection)
{
  if (!CheckExitTransition(ref_now, ref_last))
    return false;

  if (ScoreLastExit()) {
    ClearSampleAllButLast(ref_last, projection);
    state_entered = ref_last;
  }

  has_exited = true;

  return true;
}

const GeoPoint &
ScoredTaskPoint::GetLocationScored() const
{
  if (IsBoundaryScored() || !HasEntered())
    return GetLocationMin();

  return GetLocation();
}

void 
ScoredTaskPoint::Reset()
{
  SampledTaskPoint::Reset();
  state_entered.time = -1;
  has_exited = false;
}
