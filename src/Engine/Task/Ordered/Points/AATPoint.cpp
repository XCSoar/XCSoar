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

#include "AATPoint.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/Flat/FlatLine.hpp"
#include "Util/Clamp.hpp"

const GeoPoint&
AATPoint::GetLocationRemaining() const
{
  if (!IsPast())
    return target_location;

  if (HasSampled())
    return GetLocationMax();

  return GetLocationMin();
}

bool
AATPoint::UpdateSampleNear(const AircraftState& state,
                           const FlatProjection &projection)
{
  bool retval = OrderedTaskPoint::UpdateSampleNear(state, projection);
  retval |= CheckTarget(state, false);

  return retval;
}

bool
AATPoint::UpdateSampleFar(const AircraftState& state,
                          const FlatProjection &projection)
{
  /* the orderedtaskpoint::update_sample_far does nothing for now but
     we are calling this in case that changes */
  return OrderedTaskPoint::UpdateSampleFar(state, projection) ||
    CheckTarget(state, true);
}

bool
AATPoint::CheckTarget(const AircraftState &state, const bool known_outside)
{
  if (IsCurrent() && target_locked)
    return false;

  bool moved = false;
  if (!known_outside && IsInSector(state))
    moved = CheckTargetInside(state);
  else
    moved = CheckTargetOutside(state);

  return moved;
}

bool
AATPoint::IsCloseToTarget(const AircraftState& state, const double threshold) const
{
  if (!valid())
    return false;

  return DoubleLegDistance(state.location)
    - DoubleLegDistance(target_location) > -threshold;
}

bool
AATPoint::CheckTargetInside(const AircraftState& state)
{
  /* target must be moved if d(p_last,t)+d(t,p_next) <
     d(p_last,state)+d(state,p_next) */

  if (!IsCloseToTarget(state))
    return false;

  if (DoubleLegDistance(state.location) > DoubleLegDistance(GetLocationMax()))
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
AATPoint::SetRange(const double p, const bool force_if_current)
{
  if (target_locked)
    return false;

  switch (GetActiveState()) {
  case BEFORE_ACTIVE:
    return false;

  case CURRENT_ACTIVE:
    if (!HasEntered() || force_if_current) {
      target_location = InterpolateLocationMinMax(p);
      return true;
    }
    return false;

  case AFTER_ACTIVE:
    target_location = InterpolateLocationMinMax(p);
    return true;
  }

  assert(false);
  gcc_unreachable();
}

void
AATPoint::SetTarget(const GeoPoint &loc, const bool override_lock)
{
  if (override_lock || !target_locked)
    target_location = loc;
}

void
AATPoint::SetTarget(RangeAndRadial rar, const FlatProjection &proj)
{
  const auto fprev =
    proj.ProjectFloat(GetPrevious()->GetLocationRemaining());
  const auto floc = proj.ProjectFloat(GetLocation());
  const FlatLine flb (fprev,floc);
  const FlatLine fradius(floc,
                         proj.ProjectFloat(rar.range < 0
                                           ? GetLocationMin()
                                           : GetLocationMax()));
  const auto radius = fradius.GetDistance() * fabs(rar.range);

  const auto angle = rar.radial - flb.GetAngle();

  const FlatPoint ftarget1(radius * angle.cos(),
                           radius * -(angle).sin());

  const auto ftarget2 = floc + ftarget1;
  const auto targetG = proj.Unproject(ftarget2);

  SetTarget(targetG, true);
}

RangeAndRadial
AATPoint::GetTargetRangeRadial(double oldrange) const
{
  const auto fprev = GetPrevious()->GetLocationRemaining();
  const auto floc = GetLocation();
  const auto radialraw = (floc.Bearing(GetTargetLocation()) -
      fprev.Bearing(floc)).AsBearing();
  auto radial = radialraw.AsDelta();

  auto d = floc.Distance(GetTargetLocation());
  if (radial < -Angle::QuarterCircle() || radial > Angle::QuarterCircle())
    d = -d;

  const auto radius = d < 0
    ? floc.Distance(GetLocationMin())
    : floc.Distance(GetLocationMax());
  const auto range = Clamp(d / radius, -1., 1.);

  if (oldrange == 0 && range == 0)
    radial = Angle::Zero();

  return RangeAndRadial{ range, radial };
}

bool
AATPoint::Equals(const OrderedTaskPoint &other) const
{
  const auto &tp = (const AATPoint &)other;

  return OrderedTaskPoint::Equals(other) &&
    target_locked == tp.target_locked &&
    target_location == tp.target_location;
}
