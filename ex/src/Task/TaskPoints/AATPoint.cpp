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
#include "Math/Geometry.hpp"
#include <math.h>

const GEOPOINT&
AATPoint::get_location_remaining() const
{
  if (getActiveState() == BEFORE_ACTIVE) {
    return get_location_max();
  } else {
    return m_target_location;
  }
}

bool 
AATPoint::update_sample(const AIRCRAFT_STATE& state,
                        const TaskEvents &task_events) 
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
AATPoint::close_to_target(const AIRCRAFT_STATE& state, const double threshold) const
{
  return (double_leg_distance(m_target_location)-double_leg_distance(state.Location)
          <= threshold);
}

bool
AATPoint::check_target_inside(const AIRCRAFT_STATE& state) 
{
  // target must be moved if d(p_last,t)+d(t,p_next) 
  //    < d(p_last,state)+d(state,p_next)

  if (close_to_target(state)) {
    const double d_to_max = state.Location.distance(get_location_max());
    if (d_to_max<=0.0) {
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
AATPoint::set_range(const double p, const bool force_if_current)
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
AATPoint::set_target(const GEOPOINT &loc, const bool override_lock)
{
  if (override_lock || !m_target_locked) {
    m_target_location = loc;
  }
}


bool
AATPoint::equals(const OrderedTaskPoint* other) const
{
  if (const AATPoint* tp = dynamic_cast<const AATPoint*>(other)) {
    if ((m_target_locked == tp->m_target_locked)
        && (m_target_location == tp->m_target_location)) {
      return OrderedTaskPoint::equals(other);
    }
  } 
  return false;
}
