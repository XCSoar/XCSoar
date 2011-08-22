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

#include "AATPoint.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Navigation/Flat/FlatLine.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"
#include <math.h>
#include <assert.h>

const GeoPoint&
AATPoint::GetLocationRemaining() const
{
  if (getActiveState() == BEFORE_ACTIVE) {
    if (HasSampled()) {
      return GetLocationMax();
    } else {
      return GetLocationMin();
    }
  } else {
    return m_target_location;
  }
}

bool 
AATPoint::UpdateSampleNear(const AircraftState& state,
                             TaskEvents &task_events,
                             const TaskProjection &projection)
{
  bool retval = OrderedTaskPoint::UpdateSampleNear(state, task_events,
                                                     projection);

  if (retval)
    update_deadzone(projection);

  retval |= check_target(state, false);

  return retval;
}


bool 
AATPoint::UpdateSampleFar(const AircraftState& state,
                            TaskEvents &task_events,
                            const TaskProjection &projection)
{
  // the orderedtaskpoint::update_sample_far does nothing for now
  // but we are calling this in case that changes.
  return OrderedTaskPoint::UpdateSampleFar(state, task_events,
                                             projection)
    || check_target(state, true);
}


bool
AATPoint::check_target(const AircraftState& state, const bool known_outside) 
{
  if ((getActiveState() == CURRENT_ACTIVE) && (m_target_locked)) {
    return false;
  }
  bool moved = false;
  if (!known_outside && isInSector(state)) {
    moved = check_target_inside(state);
  } else {
    moved = check_target_outside(state);
  }

  return moved;
}

bool 
AATPoint::close_to_target(const AircraftState& state, const fixed threshold) const
{
  if (!valid())
    return false;

  return (double_leg_distance(state.location)
          -double_leg_distance(m_target_location)
          > -threshold);
}

bool
AATPoint::check_target_inside(const AircraftState& state) 
{
  // target must be moved if d(p_last,t)+d(t,p_next) 
  //    < d(p_last,state)+d(state,p_next)

  if (close_to_target(state)) {
    if (positive(double_leg_distance(state.location)
                 -double_leg_distance(GetLocationMax()))) {
      // no improvement available
      return false;
    } else {
      m_target_location = state.location;
      return true;
    }
  } else {
    return false;
  }
}

bool
AATPoint::check_target_outside(const AircraftState& state) 
{
  return false;
/*
  // this is optional, to be replaced!
  
  // now uses TaskOptTarget

  if (!get_previous()->isInSector(state)) {
    double b0s = get_previous()->get_location_remaining()
      .bearing(state.Location);
    GeoVector vst(state.Location,m_target_location);
    double da = ::AngleLimit180(b0s-vst.Bearing);
    if ((fabs(da)>2.0) && (vst.Distance>1.0)) {
      AATIsolineIntercept ai(*this);
      AIRCRAFT_STATE si;
      if (ai.intercept(*this, state, 0.0, si.Location)
          && isInSector(si)) {

        // Note that this fights with auto-target

        m_target_location = si.Location;

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
  if (m_target_locked) {
    return false;
  }

  switch (getActiveState()) {
  case CURRENT_ACTIVE:
    if (!HasEntered() || force_if_current) {
      m_target_location = GetLocationMin().interpolate(GetLocationMax(),p);
      return true;
    }
    return false;
  case AFTER_ACTIVE:
    if (getActiveState() == AFTER_ACTIVE) {
      m_target_location = GetLocationMin().interpolate(GetLocationMax(),p);
      return true;
    }
  default:
    return false;
  }
  return false;
}


void 
AATPoint::set_target(const GeoPoint &loc, const bool override_lock)
{
  if (override_lock || !m_target_locked) {
    m_target_location = loc;
  }
}

void
AATPoint::set_target(const fixed range, const fixed radial,
                     const TaskProjection &proj)
{
  fixed oldrange = fixed_zero;
  fixed oldradial = fixed_zero;
  get_target_range_radial(oldrange, oldradial);

  const FlatPoint fprev = proj.fproject(get_previous()->GetLocationRemaining());
  const FlatPoint floc = proj.fproject(GetLocation());
  const FlatLine flb (fprev,floc);
  const FlatLine fradius (floc,proj.fproject(GetLocationMin()));
  const fixed bearing = fixed_minus_one * flb.angle().value_degrees();
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
  const GeoPoint targetG = proj.funproject(ftarget2);

  set_target(targetG, true);
}

void
AATPoint::get_target_range_radial(fixed &range, fixed &radial) const
{
  const fixed oldrange = range;

  const GeoPoint fprev = get_previous()->GetLocationRemaining();
  const GeoPoint floc = GetLocation();
  const Angle radialraw = (floc.bearing(get_location_target()) -
      fprev.bearing(floc)).as_bearing();

  const fixed d = floc.distance(get_location_target());
  const fixed radius = floc.distance(GetLocationMin());
  const fixed rangeraw = min(fixed_one, d / radius);

  radial = radialraw.as_delta().value_degrees();
  const fixed rangesign = (fabs(radial) > fixed(90)) ?
      fixed_minus_one : fixed_one;
  range = rangeraw * rangesign;

  if ((oldrange == fixed_zero) && (range == fixed_zero))
    radial = fixed_zero;
}


bool
AATPoint::equals(const OrderedTaskPoint* other) const
{
  const AATPoint &tp = (const AATPoint &)*other;

  return OrderedTaskPoint::equals(other) &&
    m_target_locked == tp.m_target_locked &&
    m_target_location == tp.m_target_location;
}

void
AATPoint::Reset()
{
  IntermediateTaskPoint::Reset();
  m_deadzone.clear();
}

void
AATPoint::update_deadzone(const TaskProjection &projection)
{
  // deadzone must be updated

  assert(get_next());
  assert(get_previous());
  
  // the deadzone is the convex hull formed from the sampled points
  // with the inclusion of all points on the boundary closer than
  // the max double distance to the sample polygon
  
  m_deadzone = SearchPointVector(GetSamplePoints().begin(),
                                 GetSamplePoints().end());

  // do nothing if no samples (could happen if not achieved properly)
  if (m_deadzone.empty())
    return;

  // previous and next targets
  const SearchPoint pnext(get_next()->GetLocationRemaining(),
                          projection);
  const SearchPoint pprevious(get_previous()->GetLocationRemaining(),
                              projection);

  // find max distance
  unsigned dmax = 0;
  for (SearchPointVector::const_iterator it = GetSamplePoints().begin();
       it != GetSamplePoints().end(); ++it) {
    unsigned dd = pnext.flat_distance(*it) + pprevious.flat_distance(*it);
    dmax = std::max(dd, dmax);
  }

  // now add boundary points closer than dmax
  const SearchPointVector& boundary = GetBoundaryPoints();
  for (SearchPointVector::const_iterator it = boundary.begin();
       it != boundary.end(); ++it) {
    unsigned dd = pnext.flat_distance(*it) + pprevious.flat_distance(*it);
    if (dd< dmax)
      m_deadzone.push_back(*it);
  }

  // convert to convex polygon
  m_deadzone.prune_interior();
}
