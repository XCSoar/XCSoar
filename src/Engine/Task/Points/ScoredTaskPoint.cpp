// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#include "ScoredTaskPoint.hpp"

ScoredTaskPoint::ScoredTaskPoint(const GeoPoint &location, bool b_scored) noexcept
  :SampledTaskPoint(location, b_scored)
{
  entered_state.ResetTime();
  exited_state.ResetTime();
}

bool 
ScoredTaskPoint::TransitionEnter(const AircraftState &ref_now,
                                 const AircraftState &ref_last) noexcept
{
  if (!CheckEnterTransition(ref_now, ref_last))
    return false;

  if (EntryPrecondition() && (!ScoreFirstEntry() || !HasEntered()))
    entered_state = ref_now;

  return true;
}

bool 
ScoredTaskPoint::TransitionExit(const AircraftState &ref_now,
                                const AircraftState &ref_last,
                                const FlatProjection &projection) noexcept
{
  if (!CheckExitTransition(ref_now, ref_last))
    return false;

  if (ScoreLastExit()) {
    ClearSampleAllButLast(ref_last, projection);
  }

  exited_state = ref_last;

  return true;
}

const GeoPoint &
ScoredTaskPoint::GetLocationScored() const noexcept
{
  if (IsBoundaryScored() || !HasEntered())
    return GetLocationMin();

  return GetLocation();
}

void 
ScoredTaskPoint::Reset() noexcept
{
  SampledTaskPoint::Reset();
  entered_state.ResetTime();
  exited_state.ResetTime();
}
