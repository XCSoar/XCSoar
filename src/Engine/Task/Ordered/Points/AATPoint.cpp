/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "AATPoint.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "Geo/Flat/FlatLine.hpp"

const GeoPoint&
AATPoint::GetLocationRemaining() const
{
  if (GetActiveState() != BEFORE_ACTIVE)
    return target_location;

  if (HasSampled())
    return GetLocationMax();

  return GetLocationMin();
}

bool 
AATPoint::UpdateSampleNear(const AircraftState& state,
                           TaskEvents *task_events,
                           const TaskProjection &projection)
{
  bool retval = OrderedTaskPoint::UpdateSampleNear(state, task_events,
                                                   projection);
  retval |= CheckTarget(state, false);

  return retval;
}

bool 
AATPoint::UpdateSampleFar(const AircraftState& state,
                          TaskEvents *task_events,
                          const TaskProjection &projection)
{
  // the orderedtaskpoint::update_sample_far does nothing for now
  // but we are calling this in case that changes.
  return OrderedTaskPoint::UpdateSampleFar(state, task_events, projection) ||
    CheckTarget(state, true);
}

bool
AATPoint::CheckTarget(const AircraftState &state, const bool known_outside)
{
  if (GetActiveState() == CURRENT_ACTIVE && target_locked)
    return false;

  bool moved = false;
  if (!known_outside && IsInSector(state))
    moved = CheckTargetInside(state);
  else
    moved = CheckTargetOutside(state);

  return moved;
}

bool 
AATPoint::IsCloseToTarget(const AircraftState& state, const fixed threshold) const
{
  if (!valid())
    return false;

  return DoubleLegDistance(state.location)
    - DoubleLegDistance(target_location) > -threshold;
}

bool
AATPoint::CheckTargetInside(const AircraftState& state) 
{
  // target must be moved if d(p_last,t)+d(t,p_next) 
  //    < d(p_last,state)+d(state,p_next)

  if (!IsCloseToTarget(state))
    return false;

  if (positive(DoubleLegDistance(state.location)
               - DoubleLegDistance(GetLocationMax())))
    // no improvement available
    return false;

  target_location = state.location;
  return true;
}

bool
AATPoint::CheckTargetOutside(const AircraftState& state) 
{
  return false;
/*
  // this is optional, to be replaced!
  
  // now uses TaskOptTarget

  if (!GetPrevious()->isInSector(state)) {
    double b0s = GetPrevious()->get_location_remaining()
      .bearing(state.Location);
    GeoVector vst(state.Location,target_location);
    double da = ::AngleLimit180(b0s-vst.Bearing);
    if ((fabs(da)>2.0) && (vst.Distance>1.0)) {
      AATIsolineIntercept ai(*this);
      AIRCRAFT_STATE si;
      if (ai.intercept(*this, state, 0.0, si.Location)
          && isInSector(si)) {

        // Note that this fights with auto-target

        target_location = si.Location;

        return true;
      }
    }
  }
  return false;
*/
}

bool
AATPoint::SetRange(const fixed p, const bool force_if_current)
{
  if (target_locked)
    return false;

  switch (GetActiveState()) {
  case CURRENT_ACTIVE:
    if (!HasEntered() || force_if_current) {
      target_location = GetLocationMin().Interpolate(GetLocationMax(),p);
      return true;
    }
    return false;

  case AFTER_ACTIVE:
    if (GetActiveState() == AFTER_ACTIVE) {
      target_location = GetLocationMin().Interpolate(GetLocationMax(),p);
      return true;
    }
    return false;

  default:
    return false;
  }
}

void 
AATPoint::SetTarget(const GeoPoint &loc, const bool override_lock)
{
  if (override_lock || !target_locked)
    target_location = loc;
}

void
AATPoint::SetTarget(const fixed range, const fixed radial,
                    const TaskProjection &proj)
{
  fixed oldrange = fixed_zero;
  fixed oldradial = fixed_zero;
  GetTargetRangeRadial(oldrange, oldradial);

  const FlatPoint fprev =
    proj.ProjectFloat(GetPrevious()->GetLocationRemaining());
  const FlatPoint floc = proj.ProjectFloat(GetLocation());
  const FlatLine flb (fprev,floc);
  const FlatLine fradius (floc,proj.ProjectFloat(GetLocationMin()));
  const fixed bearing = fixed_minus_one * flb.angle().Degrees();
  const fixed radius = fradius.d();

  fixed swapquadrants = fixed_zero;
  if (positive(range) != positive(oldrange))
    swapquadrants = fixed(180);
  const FlatPoint ftarget1 (fabs(range) * radius *
        cos((bearing + radial + swapquadrants)
            / fixed(360) * fixed_two_pi),
      fabs(range) * radius *
        sin( fixed_minus_one * (bearing + radial + swapquadrants)
            / fixed(360) * fixed_two_pi));

  const FlatPoint ftarget2 = floc + ftarget1;
  const GeoPoint targetG = proj.Unproject(ftarget2);

  SetTarget(targetG, true);
}

void
AATPoint::GetTargetRangeRadial(fixed &range, fixed &radial) const
{
  const fixed oldrange = range;

  const GeoPoint fprev = GetPrevious()->GetLocationRemaining();
  const GeoPoint floc = GetLocation();
  const Angle radialraw = (floc.Bearing(GetTargetLocation()) -
      fprev.Bearing(floc)).AsBearing();

  const fixed d = floc.Distance(GetTargetLocation());
  const fixed radius = floc.Distance(GetLocationMin());
  const fixed rangeraw = min(fixed_one, d / radius);

  radial = radialraw.AsDelta().Degrees();
  const fixed rangesign = (fabs(radial) > fixed(90)) ?
      fixed_minus_one : fixed_one;
  range = rangeraw * rangesign;

  if ((oldrange == fixed_zero) && (range == fixed_zero))
    radial = fixed_zero;
}

bool
AATPoint::Equals(const OrderedTaskPoint &other) const
{
  const AATPoint &tp = (const AATPoint &)other;

  return OrderedTaskPoint::Equals(other) &&
    target_locked == tp.target_locked &&
    target_location == tp.target_location;
}
