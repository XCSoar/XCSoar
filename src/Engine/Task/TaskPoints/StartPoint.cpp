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

#include "StartPoint.hpp"
#include "Task/OrderedTaskBehaviour.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/TaskEvents.hpp"
#include "Util/ZeroFinder.hpp"
#include "Math/Earth.hpp"

#include <assert.h>

StartPoint::StartPoint(ObservationZonePoint* _oz,
                       const Waypoint & wp,
                       const TaskBehaviour& tb,
                       const OrderedTaskBehaviour& to): 
  OrderedTaskPoint(START, _oz, wp, to),
  margins(tb),
  enabled(true) 
{
}

void
StartPoint::SetTaskBehaviour(const TaskBehaviour &tb)
{
  margins = tb;
}

fixed
StartPoint::get_elevation() const
{
  // no need for safety height at start?
  return GetBaseElevation();
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
StartPoint::update_sample_near(const AIRCRAFT_STATE& state,
                               TaskEvents &task_events,
                               const TaskProjection &projection)
{
  if (isInSector(state)) {
    if (!m_ordered_task_behaviour.check_start_speed(state, margins)) {
      task_events.warning_start_speed();
    }
  }
  return OrderedTaskPoint::update_sample_near(state, task_events, projection);
}

void 
StartPoint::find_best_start(const AIRCRAFT_STATE &state,
                            const OrderedTaskPoint &next,
                            const TaskProjection &projection)
{
  class StartPointBestStart: public ZeroFinder {
    const StartPoint& m_start;
    const GeoPoint m_loc_from;
    const GeoPoint m_loc_to;
    fixed p_offset;

  public:
    StartPointBestStart(const StartPoint& ts,
                        const GeoPoint &loc_from,
                        const GeoPoint &loc_to):
      ZeroFinder(-fixed_half, fixed_half, fixed(0.01)),
      m_start(ts),
      m_loc_from(loc_from),
      m_loc_to(loc_to) {};

    fixed f(const fixed p) {
      return ::DoubleDistance(m_loc_from,parametric(p),m_loc_to);
    }

    GeoPoint solve() {
      // find approx solution first, being the offset for the local function
      // minimiser search
      p_offset = fixed_zero;
      fixed f_best= f(fixed_zero);
      for (; p_offset < fixed_one; p_offset += fixed(0.25)) {
        fixed ff = f(fixed_zero);
        if (ff< f_best) {
          f_best = ff;
        }
      }
      // now detailed search, returning result
      return parametric(find_min(fixed_zero));
    }
  private:
    GeoPoint parametric(const fixed p) {
      // ensure parametric input is between 0 and 1
      fixed pp = p+p_offset;
      if (negative(pp)) {
        pp+= fixed_one;
      }
      pp = fmod(pp,fixed_one);
      return m_start.get_boundary_parametric(pp);
    }
  };

  StartPointBestStart solver(*this, state.Location,
                             next.get_location_remaining());
  set_search_min(solver.solve(), projection);
}


bool 
StartPoint::isInSector(const AIRCRAFT_STATE &state) const
{
  if (!ObservationZoneClient::isInSector(state)) 
    return false;

  return m_ordered_task_behaviour.check_start_height(state, margins,
                                                     GetBaseElevation());
}

bool 
StartPoint::check_transition_exit(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last) const
{
  const bool now_in_height = 
    m_ordered_task_behaviour.check_start_height(ref_now,
                                                margins,
                                                GetBaseElevation());
  const bool last_in_height = 
    m_ordered_task_behaviour.check_start_height(ref_last,
                                                margins,
                                                GetBaseElevation());

  if (now_in_height && last_in_height) {
    // both within height limit, so use normal location checks
    return ObservationZone::check_transition_exit(ref_now, ref_last);
  }
  if (!transition_constraint(ref_now, ref_last)) {
    // don't allow vertical crossings for line OZ's
    return false;
  }

  // transition inside sector to above 
  return !now_in_height && last_in_height 
    && ObservationZoneClient::isInSector(ref_last)
    && ObservationZoneClient::canStartThroughTop();
}
