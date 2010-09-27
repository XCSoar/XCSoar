/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Navigation/Flat/FlatLine.hpp"
#include <math.h>

const GeoPoint&
AATPoint::get_location_remaining() const
{
  if (getActiveState() == BEFORE_ACTIVE) {
    if (has_sampled()) {
      return get_location_max();
    } else {
      return get_location_min();
    }
  } else {
    return m_target_location;
  }
}

bool 
AATPoint::update_sample(const AIRCRAFT_STATE& state,
                        TaskEvents &task_events) 
{
  bool retval = OrderedTaskPoint::update_sample(state,task_events);
  if ((getActiveState() == CURRENT_ACTIVE) && (!m_target_locked)) {
    retval |= check_target(state);
  }

  return retval;
}

bool
AATPoint::check_target(const AIRCRAFT_STATE& state) 
{
  bool moved = false;
  if (isInSector(state)) {
    moved = check_target_inside(state);
  } else {
    moved = check_target_outside(state);
  }

  return moved;
}

bool 
AATPoint::close_to_target(const AIRCRAFT_STATE& state, const fixed threshold) const
{
  if (!valid())
    return false;

  return (double_leg_distance(m_target_location)-double_leg_distance(state.Location)
          <= threshold);
}

bool
AATPoint::check_target_inside(const AIRCRAFT_STATE& state) 
{
  // target must be moved if d(p_last,t)+d(t,p_next) 
  //    < d(p_last,state)+d(state,p_next)

  if (close_to_target(state)) {
    const fixed d_to_max = state.Location.distance(get_location_max());
    if (d_to_max <= fixed_zero) {
      // no improvement available
      return false;
    } else {
      m_target_location = state.Location;
      return true;
    }
  } else {
    return false;
  }
}

bool
AATPoint::check_target_outside(const AIRCRAFT_STATE& state) 
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
AATPoint::set_range(const fixed p, const bool force_if_current)
{
  if (m_target_locked) {
    return false;
  }

  switch (getActiveState()) {
  case CURRENT_ACTIVE:
    if (!has_entered() || force_if_current) {
      m_target_location = get_location_min().interpolate(get_location_max(),p);
      return true;
    }
    return false;
  case AFTER_ACTIVE:
    if (getActiveState() == AFTER_ACTIVE) {
      m_target_location = get_location_min().interpolate(get_location_max(),p);
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
AATPoint::set_target(const fixed range, const fixed radial)
{
  fixed oldrange = fixed_zero;
  fixed oldradial = fixed_zero;
  get_target_range_radial(oldrange, oldradial);

  const TaskProjection &proj = get_task_projection();

  const FlatPoint fprev = proj.fproject(get_previous()->get_location());
  const FlatPoint floc = proj.fproject(get_location());
  const FlatLine flb (fprev,floc);
  const FlatLine fradius (floc,proj.fproject(get_location_min()));
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

  const GeoPoint fprev = get_previous()->get_location();
  const GeoPoint floc = get_location();
  const Angle radialraw = (floc.bearing(get_location_target()) -
      fprev.bearing(floc)).as_bearing();

  const fixed d = floc.distance(get_location_target());
  const fixed radius = floc.distance(get_location_min());
  const fixed rangeraw = d / radius;
  if (d > radius)
    return; // should never happen

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
