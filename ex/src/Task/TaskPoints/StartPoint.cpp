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

#include "StartPoint.hpp"
#include "Task/TaskEvents.hpp"
#include <assert.h>

StartPoint::StartPoint(ObservationZonePoint* _oz,
                       const TaskProjection& tp,
                       const Waypoint & wp,
                       const TaskBehaviour& tb) : 
  OrderedTaskPoint(_oz,tp,wp,tb), 
  enabled(true) 
{
}


fixed
StartPoint::get_elevation() const
{
  // no need for safety height at start?
  return m_elevation;
}


void 
StartPoint::set_neighbours(OrderedTaskPoint* _prev,
                           OrderedTaskPoint* _next)
{
  assert(_prev==NULL);
  // should not ever have an inbound leg
  OrderedTaskPoint::set_neighbours(_prev, _next);
}


bool 
StartPoint::update_sample(const AIRCRAFT_STATE& state,
                          const TaskEvents &task_events)
{
  if (isInSector(state)) {
    if (!m_task_behaviour.check_start_speed(state)) {
      task_events.warning_start_speed();
    }
  }
  return OrderedTaskPoint::update_sample(state, task_events);
}


bool 
StartPoint::equals(const OrderedTaskPoint* other) const
{
  if (dynamic_cast<const StartPoint*>(other)) {
    return OrderedTaskPoint::equals(other);
  } else {
    return false;
  }
}
