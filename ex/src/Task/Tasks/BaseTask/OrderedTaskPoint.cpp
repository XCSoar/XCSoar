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
#include "OrderedTaskPoint.hpp"
#include "Math/Earth.hpp"
#include <assert.h>
#include <math.h>


OrderedTaskPoint::OrderedTaskPoint(ObservationZonePoint* _oz,
                                   const TaskProjection& tp,
                                   const Waypoint & wp, 
                                   const TaskBehaviour &tb,
                                   const bool b_scored): 
  TaskLeg(*this),
  ScoredTaskPoint(tp, wp, tb, b_scored),
  ObservationZoneClient(_oz),
  m_active_state(NOTFOUND_ACTIVE),
  tp_next(NULL),
  tp_previous(NULL)
{
}


OrderedTaskPoint* 
OrderedTaskPoint::get_previous() const
{
  return tp_previous;
}

OrderedTaskPoint* 
OrderedTaskPoint::get_next() const
{
  return tp_next;
}


void 
OrderedTaskPoint::set_neighbours(OrderedTaskPoint* _prev,
                                 OrderedTaskPoint* _next) 
{
  tp_previous = _prev;
  tp_next = _next;

  update_geometry();
}


/** 
 * Update observation zone geometry (or other internal data) when
 * previous/next turnpoint changes.
 */
void 
OrderedTaskPoint::update_geometry() {
  set_legs(tp_previous, this, tp_next);
}


void
OrderedTaskPoint::update_oz()
{
  update_geometry();

  /// \todo also clear search points?
  SampledTaskPoint::update_oz();
}


bool 
OrderedTaskPoint::scan_active(OrderedTaskPoint* atp) 
{
  // reset
  m_active_state = NOTFOUND_ACTIVE;

  if (atp == this) {
    m_active_state = CURRENT_ACTIVE;
  } else if (tp_previous 
             && ((get_previous()->getActiveState() 
                  == CURRENT_ACTIVE) 
                 || (get_previous()->getActiveState() 
                     == AFTER_ACTIVE))) {
    m_active_state = AFTER_ACTIVE;
  } else {
    m_active_state = BEFORE_ACTIVE;
  }

  if (tp_next) { 
    // propagate to remainder of task
    return get_next()->scan_active(atp);
  } else {
    return (m_active_state != BEFORE_ACTIVE) && (m_active_state != NOTFOUND_ACTIVE);
  }
}


bool
OrderedTaskPoint::search_boundary_points() const
{
  return m_active_state == AFTER_ACTIVE;
}

bool
OrderedTaskPoint::search_nominal_if_unsampled() const
{
  return m_active_state == BEFORE_ACTIVE;
}

fixed 
OrderedTaskPoint::double_leg_distance(const GEOPOINT &ref) const
{
  assert(tp_previous);
  assert(tp_next);

  return ::DoubleDistance(get_previous()->get_location_remaining(), 
                          ref, 
                          get_next()->get_location_remaining());
}


bool 
OrderedTaskPoint::equals(const OrderedTaskPoint* other) const
{
  return (get_waypoint() == other->get_waypoint()) &&
    get_oz()->equals(other->get_oz());
}

