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

#include "UnorderedTask.hpp"
#include "TaskSolvers/TaskBestMc.hpp"
#include "TaskSolvers/TaskGlideRequired.hpp"
#include "TaskSolvers/TaskSolution.hpp"

UnorderedTask::UnorderedTask(const TaskEvents &te, 
                             const TaskBehaviour &tb,
                             TaskAdvance &ta,
                             GlidePolar &gp):
  AbstractTask(te, tb, ta, gp) 
{
}


double 
UnorderedTask::calc_mc_best(const AIRCRAFT_STATE &aircraft)
{
  TaskPoint *tp = getActiveTaskPoint();
  if (!tp) {
    return glide_polar.get_mc();
  }
  TaskBestMc bmc(tp, aircraft, glide_polar);
  return bmc.search(glide_polar.get_mc());
}

bool
UnorderedTask::check_task() const
{
  return (getActiveTaskPoint()!=NULL);
}

double 
UnorderedTask::calc_glide_required(const AIRCRAFT_STATE &aircraft)
{
  TaskPoint *tp = getActiveTaskPoint();
  if (!tp) {
    return 0.0;
  }
  TaskGlideRequired bgr(tp, aircraft, glide_polar);
  return bgr.search(fixed_zero);
}

void
UnorderedTask::glide_solution_remaining(const AIRCRAFT_STATE &state, 
                                        const GlidePolar &polar,
                                        GlideResult &total,
                                        GlideResult &leg)
{
  GlideResult res;

  TaskPoint* tp = getActiveTaskPoint();
  if (tp) {
    res = TaskSolution::glide_solution_remaining(*tp, state, polar);
    res.calc_deferred(state);
  }
  total = res;
  leg = res;
}

void 
UnorderedTask::glide_solution_travelled(const AIRCRAFT_STATE &state, 
                                        GlideResult &total,
                                        GlideResult &leg)
{
  GlideResult null_res;
  total = null_res;
  leg = null_res;
}

void 
UnorderedTask::glide_solution_planned(const AIRCRAFT_STATE &state, 
                                      GlideResult &total,
                                      GlideResult &leg,
                                      DistanceRemainingStat &total_remaining_effective,
                                      DistanceRemainingStat &leg_remaining_effective,
                                      const double total_t_elapsed,
                                      const double leg_t_elapsed)
{
  GlideResult res = stats.total.solution_remaining;
  total = res;
  leg = res;
  total_remaining_effective.set_distance(res.Vector.Distance);
  leg_remaining_effective.set_distance(res.Vector.Distance);
}



double 
UnorderedTask::scan_total_start_time(const AIRCRAFT_STATE &state)
{
  return state.Time;
}

double 
UnorderedTask::scan_leg_start_time(const AIRCRAFT_STATE &state)
{
  return state.Time;
}


void 
UnorderedTask::scan_distance_minmax(const GEOPOINT &location, bool full,
                                    fixed *dmin, fixed *dmax)
{
  *dmin = stats.total.remaining.get_distance();
  *dmax = stats.total.remaining.get_distance();
}

fixed 
UnorderedTask::scan_distance_nominal()
{
  return fixed(stats.total.remaining.get_distance());
}

fixed
UnorderedTask::scan_distance_planned()
{
  return fixed(stats.total.remaining.get_distance());
}

fixed 
UnorderedTask::scan_distance_scored(const GEOPOINT &location)
{
  return fixed_zero;
}

fixed 
UnorderedTask::scan_distance_travelled(const GEOPOINT &location)
{
  return fixed_zero;
}

fixed 
UnorderedTask::scan_distance_remaining(const GEOPOINT &location)
{
  TaskPoint *tp = getActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  return tp->distance(location);
}

double 
UnorderedTask::calc_gradient(const AIRCRAFT_STATE &state) 
{
  return leg_gradient(state);
}

