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

#include "StartPoint.hpp"
#include "Task/Ordered/OrderedTaskBehaviour.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/TaskEvents.hpp"
#include "Math/ZeroFinder.hpp"
#include "Geo/Math.hpp"

#include <assert.h>

StartPoint::StartPoint(ObservationZonePoint *_oz,
                       const Waypoint &wp,
                       const TaskBehaviour &tb,
                       const StartConstraints &_constraints)
  :OrderedTaskPoint(TaskPointType::START, _oz, wp, false),
   safety_height_terrain(tb.route_planner.safety_height_terrain),
   margins(tb.start_margins),
   constraints(_constraints)
{
}

void
StartPoint::SetTaskBehaviour(const TaskBehaviour &tb)
{
  safety_height_terrain = tb.route_planner.safety_height_terrain;
  margins = tb.start_margins;
}

fixed
StartPoint::GetElevation() const
{
  return GetBaseElevation() + safety_height_terrain;
}

void
StartPoint::SetOrderedTaskBehaviour(const OrderedTaskBehaviour &otb)
{
  OrderedTaskPoint::SetOrderedTaskBehaviour(otb);
  constraints = otb.start_constraints;
}

void
StartPoint::SetNeighbours(OrderedTaskPoint *_prev, OrderedTaskPoint *_next)
{
  assert(_prev==NULL);
  // should not ever have an inbound leg
  OrderedTaskPoint::SetNeighbours(_prev, _next);
}


bool
StartPoint::UpdateSampleNear(const AircraftState& state,
                             const TaskProjection &projection)
{
  /* TODO:
  if (IsInSector(state) && !constraints.CheckSpeed(state, margins))
    TO_BE_IMPLEMENTED;
  */

  return OrderedTaskPoint::UpdateSampleNear(state, projection);
}

void
StartPoint::find_best_start(const AircraftState &state,
                            const OrderedTaskPoint &next,
                            const TaskProjection &projection)
{
  class StartPointBestStart: public ZeroFinder {
    const StartPoint &start;
    const GeoPoint loc_from;
    const GeoPoint loc_to;
    fixed p_offset;

  public:
    StartPointBestStart(const StartPoint& ts,
                        const GeoPoint &_loc_from,
                        const GeoPoint &_loc_to):
      ZeroFinder(-fixed(0.5), fixed(0.5), fixed(0.01)),
      start(ts),
      loc_from(_loc_from),
      loc_to(_loc_to) {};

    virtual fixed f(const fixed p) {
      return ::DoubleDistance(loc_from, parametric(p), loc_to);
    }

    GeoPoint solve() {
      // find approx solution first, being the offset for the local function
      // minimiser search
      p_offset = fixed(0);
      fixed f_best= f(fixed(0));
      for (; p_offset < fixed(1); p_offset += fixed(0.25)) {
        fixed ff = f(fixed(0));
        if (ff< f_best) {
          f_best = ff;
        }
      }
      // now detailed search, returning result
      return parametric(find_min(fixed(0)));
    }
  private:
    GeoPoint parametric(const fixed p) {
      // ensure parametric input is between 0 and 1
      fixed pp = p+p_offset;
      if (negative(pp)) {
        pp+= fixed(1);
      }
      pp = fmod(pp,fixed(1));
      return start.GetBoundaryParametric(pp);
    }
  };

  StartPointBestStart solver(*this, state.location,
                             next.GetLocationRemaining());
  SetSearchMin(SearchPoint(solver.solve(), projection));
}

bool
StartPoint::IsInSector(const AircraftState &state) const
{
  return OrderedTaskPoint::IsInSector(state) &&
    constraints.CheckHeight(state, margins, GetBaseElevation());
}

bool
StartPoint::CheckExitTransition(const AircraftState &ref_now,
                                const AircraftState &ref_last) const
{
  const bool now_in_height =
    constraints.CheckHeight(ref_now, margins, GetBaseElevation());
  const bool last_in_height =
    constraints.CheckHeight(ref_last, margins, GetBaseElevation());

  if (now_in_height && last_in_height) {
    // both within height limit, so use normal location checks
    return OrderedTaskPoint::CheckExitTransition(ref_now, ref_last);
  }
  if (!TransitionConstraint(ref_now.location, ref_last.location)) {
    // don't allow vertical crossings for line OZ's
    return false;
  }

  // transition inside sector to above
  return !now_in_height && last_in_height
    && OrderedTaskPoint::IsInSector(ref_last)
    && CanStartThroughTop();
}
