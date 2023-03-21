// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
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
  state_entered.ResetTime();
  has_exited = false;
}
