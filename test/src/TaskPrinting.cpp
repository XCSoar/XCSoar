/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "Printing.hpp"
#include <fstream>

#include "Engine/Task/TaskManager.hpp"
#include "Task/Tasks/AbortTask.hpp"
#include "Task/Tasks/GotoTask.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Task/Tasks/BaseTask/TaskPoint.hpp"
#include "Task/Tasks/BaseTask/SampledTaskPoint.hpp"
#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"


#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"

void 
PrintHelper::aatpoint_print(std::ostream& f, 
                            const AATPoint& tp,
                            const AircraftState& state,
                            const TaskProjection &projection,
                            const int item) 
{
  switch(item) {
  case 0:
    orderedtaskpoint_print(f, tp, state, item);
    f << "#   Target " << tp.m_target_location.longitude << ","
      << tp.m_target_location.latitude << "\n";
    break;

  case 1:

    if (tp.valid() && (tp.getActiveState() != OrderedTaskPoint::BEFORE_ACTIVE)) {
      assert(tp.get_previous());
      assert(tp.get_next());
      // note in general this will only change if 
      // prev max or target changes

      AATIsolineSegment seg(tp, projection);
      fixed tdist = tp.get_previous()->GetLocationRemaining().Distance(
        tp.GetLocationMin());
      fixed rdist = tp.get_previous()->GetLocationRemaining().Distance(
        tp.get_location_target());

      bool filter_backtrack = true;
      if (seg.IsValid()) {
        for (double t = 0.0; t<=1.0; t+= 1.0/20) {
          GeoPoint ga = seg.Parametric(fixed(t));
          fixed dthis = tp.get_previous()->GetLocationRemaining().Distance(ga);
          if (!filter_backtrack 
              || (dthis>=tdist)
              || (dthis>=rdist)) {
            /// @todo unless double dist is better than current
            f << ga.longitude << " " << ga.latitude << "\n";
          }
        }
      } else {
        GeoPoint ga = seg.Parametric(fixed_zero);
        f << ga.longitude << " " << ga.latitude << "\n";
      }
      f << "\n";

    }
    break;
  }
}


void 
PrintHelper::orderedtaskpoint_print(std::ostream& f, 
                                    const OrderedTaskPoint& tp,
                                    const AircraftState& state,
                                    const int item) 
{
  if (item==0) {
    sampledtaskpoint_print(f,tp,state);
    orderedtaskpoint_print_boundary(f,tp,state);
    f << "# Entered " << tp.GetEnteredState().time << "\n";
    f << "# Bearing travelled " << tp.vector_travelled.Bearing << "\n";
    f << "# Distance travelled " << tp.vector_travelled.Distance << "\n";
    f << "# Bearing remaining " << tp.vector_remaining.Bearing << "\n";
    f << "# Distance remaining " << tp.vector_remaining.Distance << "\n";
    f << "# Bearing planned " << tp.vector_planned.Bearing << "\n";
    f << "# Distance planned " << tp.vector_planned.Distance << "\n";
  }
}


void 
PrintHelper::orderedtaskpoint_print_boundary(std::ostream& f, 
                                             const OrderedTaskPoint& tp,
                                             const AircraftState &state) 
{
  f << "#   Boundary points\n";
  for (double t=0; t<= 1.0; t+= 0.05) {
    GeoPoint loc = tp.GetBoundaryParametric(fixed(t));
    f << "     " << loc.longitude << " " << loc.latitude << "\n";
  }
  GeoPoint loc = tp.GetBoundaryParametric(fixed_zero);
  f << "     " << loc.longitude << " " << loc.latitude << "\n";
  f << "\n";
}


void 
PrintHelper::sampledtaskpoint_print(std::ostream& f, const SampledTaskPoint& tp,
                                    const AircraftState &state) 
{
  taskpoint_print(f,tp,state);
}


void 
PrintHelper::sampledtaskpoint_print_samples(std::ostream& f,
                                            const SampledTaskPoint& tp,
                                            const AircraftState &state) 
{
  const unsigned n= tp.GetSearchPoints().size();
  f << "#   Search points\n";
  if (tp.HasEntered()) {
    for (unsigned i=0; i<n; i++) {
      const GeoPoint loc = tp.GetSearchPoints()[i].get_location();
      f << "     " << loc.longitude << " " << loc.latitude << "\n";
    }
  }
  f << "\n";
}


void 
PrintHelper::taskpoint_print(std::ostream& f, const TaskPoint& tp,
                             const AircraftState &state) 
{
  f << "# Task point \n";
  f << "#   Location " << tp.GetLocation().longitude << "," <<
    tp.GetLocation().latitude << "\n";
}


void
PrintHelper::abstracttask_print(AbstractTask& task, const AircraftState &state) 
{
  std::ofstream fs("results/res-stats-all.txt");
  if (!task.stats.task_valid)
    return;

  fs << task.stats;

  static std::ofstream f6("results/res-stats.txt");
  static bool first = true;

  if (first) {
    first = false;
    f6 << "# Time atp mc_best d_tot_rem_eff d_tot_rem ceff v_tot_rem v_tot_rem_inc v_tot_eff v_tot_eff_inc task_vario effective_mc v_pirker alt_diff\n";
  }

  if (positive(task.stats.Time)) {
    f6 << task.stats.Time
       << " " << task.activeTaskPoint
       << " " << task.stats.mc_best
       << " " << task.stats.total.remaining_effective.get_distance()
       << " " << task.stats.total.remaining.get_distance() 
       << " " << task.stats.cruise_efficiency 
       << " " << task.stats.total.remaining.get_speed() 
       << " " << task.stats.total.remaining.get_speed_incremental() 
       << " " << task.stats.total.remaining_effective.get_speed() 
       << " " << task.stats.total.remaining_effective.get_speed_incremental() 
       << " " << task.stats.total.vario.get_value() 
       << " " << task.stats.effective_mc
       << " " << task.stats.get_pirker_speed()
       << " " << task.stats.total.solution_remaining.altitude_difference
       << "\n";
    f6.flush();
  } else {
    f6 << "\n";
    f6.flush();
  }
}


void 
PrintHelper::gototask_print(GotoTask& task, const AircraftState &state) 
{
  abstracttask_print(task, state);
  if (task.tp) {
    std::ofstream f1("results/res-goto.txt");
    taskpoint_print(f1,*task.tp,state);
  }
}

void 
PrintHelper::orderedtask_print(OrderedTask& task, const AircraftState &state) 
{
  abstracttask_print(task, state);
  if (!task.stats.task_valid)
    return;

  std::ofstream fi("results/res-isolines.txt");
  for (unsigned i=0; i<task.task_points.size(); i++) {
    fi << "## point " << i << "\n";
    if (task.task_points[i]->type == TaskPoint::AAT) {
      aatpoint_print(fi, (AATPoint&)*task.task_points[i], state,
                     task.get_task_projection(), 1);
    } else {
      orderedtaskpoint_print(fi,*task.task_points[i],state,1);
    }
    fi << "\n";
  }

  std::ofstream f1("results/res-task.txt");

  f1 << "#### Task points\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    f1 << "## point " << i << " ###################\n";
    if (task.task_points[i]->type == TaskPoint::AAT) {
      aatpoint_print(f1, (AATPoint&)*task.task_points[i], state,
                     task.get_task_projection(), 0);
    } else {
      orderedtaskpoint_print(f1,*task.task_points[i],state,0);
    }
    f1 << "\n";
  }

  std::ofstream f5("results/res-ssample.txt");
  f5 << "#### Task sampled points\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    f5 << "## point " << i << "\n";
    sampledtaskpoint_print_samples(f5,*task.task_points[i],state);
    f5 << "\n";
  }

  std::ofstream f2("results/res-max.txt");
  f2 << "#### Max task\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    OrderedTaskPoint *tp = task.task_points[i];
    f2 <<  tp->GetLocationMax().longitude << " "
       <<  tp->GetLocationMax().latitude << "\n";
  }

  std::ofstream f3("results/res-min.txt");
  f3 << "#### Min task\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    OrderedTaskPoint *tp = task.task_points[i];
    f3 <<  tp->GetLocationMin().longitude << " "
       <<  tp->GetLocationMin().latitude << "\n";
  }

  std::ofstream f4("results/res-rem.txt");
  f4 << "#### Remaining task\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    OrderedTaskPoint *tp = task.task_points[i];
    f4 <<  tp->GetLocationRemaining().longitude << " "
       <<  tp->GetLocationRemaining().latitude << "\n";
  }
}


void PrintHelper::aborttask_print(AbortTask& task, const AircraftState &state)
{
  abstracttask_print(task, state);

  std::ofstream f1("results/res-abort-task.txt");
  f1 << "#### Task points\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    GeoPoint l = task.task_points[i].GetLocation();
    f1 << "## point " << i << " ###################\n";
    if (i==task.activeTaskPoint) {
      f1 << state.location.longitude << " " << state.location.latitude << "\n";
    }
    f1 << l.longitude << " " << l.latitude << "\n";
    f1 << "\n";
  }
}


void PrintHelper::taskmanager_print(TaskManager& task, const AircraftState &state)
{
  if (task.active_task) {
    if (task.active_task == &task.task_abort) {
      aborttask_print(task.task_abort, state);
    }
    if (task.active_task == &task.task_goto) {
      gototask_print(task.task_goto, state);
    }
    if (task.active_task == &task.task_ordered) {
      orderedtask_print(task.task_ordered, state);
    }
  }
}

#include "Math/Earth.hpp"
#include "Navigation/TaskProjection.hpp"



/*
std::ostream& operator<< (std::ostream& o, 
                          const TaskProjection& tp)
{
  o << "# Task projection\n";
  o << "# deg (" << tp.location_min.Longitude << ","
    << tp.location_min.Latitude << "),("
    << tp.location_max.Longitude << "," << tp.location_max.Latitude << ")\n";

  FlatGeoPoint pll, pur;
  pll = tp.project(tp.location_min);
  pur = tp.project(tp.location_max);

  o << "# flat (" << pll.Longitude << "," << pll.Latitude << "),("
    << pur.Longitude << "," << pur.Latitude << "\n";
  return o;
}
*/


#include "GlideSolvers/GlideResult.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const GlideResult& gl)
{
  if (gl.validity != GlideResult::RESULT_OK) {
    f << "#     Solution NOT OK\n";
  }
  f << "#    Altitude Difference " << gl.altitude_difference << " (m)\n";
  f << "#    Distance            " << gl.vector.Distance << " (m)\n";
  f << "#    TrackBearing        " << gl.vector.Bearing << " (deg)\n";
  f << "#    CruiseTrackBearing  " <<  gl.cruise_track_bearing << " (deg)\n";
  f << "#    VOpt                " <<  gl.v_opt << " (m/s)\n";
  f << "#    HeightClimb         " <<  gl.height_climb << " (m)\n";
  f << "#    HeightGlide         " <<  gl.height_glide << " (m)\n";
  f << "#    TimeElapsed         " <<  gl.time_elapsed << " (s)\n";
  f << "#    TimeVirtual         " <<  gl.time_virtual << " (s)\n";
  if (positive(gl.time_elapsed)) {
    f << "#    Vave remaining      " <<  gl.vector.Distance/gl.time_elapsed << " (m/s)\n";
  }
  f << "#    EffectiveWindSpeed  " <<  gl.effective_wind_speed << " (m/s)\n";
  f << "#    EffectiveWindAngle  " <<  gl.effective_wind_angle << " (deg)\n";
  f << "#    DistanceToFinal     " <<  gl.distance_to_final << " (m)\n";
  if (gl.IsFinalGlide()) {
    f << "#    On final glide\n";
  }
  return f;
}

#include "Task/TaskStats/TaskStats.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const DistanceStat& ds)
{
  f << "#    Distance " << ds.distance << " (m)\n";
  f << "#    Speed " << ds.speed << " (m/s)\n";
  f << "#    Speed incremental " << ds.speed_incremental << " (m/s)\n";
  return f;
}

std::ostream& operator<< (std::ostream& f, 
                          const TaskStats& ts)
{
  f << "#### Task Stats\n";
  f << "# dist nominal " << ts.distance_nominal << " (m)\n";
  f << "# min dist after achieving max " << ts.distance_min << " (m)\n";
  f << "# max dist after achieving max " << ts.distance_max << " (m)\n";
  f << "# dist scored " << ts.distance_scored << " (m)\n";
  f << "# mc best " << ts.mc_best << " (m/s)\n";
  f << "# cruise efficiency " << ts.cruise_efficiency << "\n";
  f << "# glide required " << ts.glide_required << "\n";
  f << "#\n";
  f << "# Total -- \n";
  f << ts.total;
  f << "# Leg -- \n";
  f << ts.current_leg;
  return f;
}


#include "Task/TaskStats/CommonStats.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const ElementStat& es)
{
  f << "#  Time started " << es.time_started << " (s)\n";
  f << "#  Time elapsed " << es.time_elapsed << " (s)\n";
  f << "#  Time remaining " << es.time_remaining << " (s)\n";
  f << "#  Time planned " << es.time_planned << " (s)\n";
  f << "#  Gradient " << es.gradient << "\n";
  f << "#  Remaining: \n";
  f << es.remaining;
  f << es.solution_remaining;
  f << "#  Remaining effective: \n";
  f << es.remaining_effective;
  f << "#  Remaining mc0: \n";
  f << es.solution_mc0;
  f << "#  Planned: \n";
  f << es.planned;
  f << es.solution_planned;
  f << "#  Travelled: \n";
  f << es.travelled;
  f << es.solution_travelled;
  f << "#  Vario: ";
  f << es.vario.get_value();
  f << "\n";
  return f;
}
