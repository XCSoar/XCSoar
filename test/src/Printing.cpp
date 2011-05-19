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
#include "Task/Tasks/ContestManager.hpp"
#include "Trace/Trace.hpp"

#ifdef FIXED_MATH
std::ostream& operator<<(std::ostream& os,fixed const& value)
{
  return os<<value.as_double();
}
#endif


#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"

void 
PrintHelper::aatpoint_print(std::ostream& f, 
                            const AATPoint& tp,
                            const AIRCRAFT_STATE& state,
                            const TaskProjection &projection,
                            const int item) 
{
  switch(item) {
  case 0:
    orderedtaskpoint_print(f, tp, state, item);
    f << "#   Target " << tp.m_target_location.Longitude << "," 
      << tp.m_target_location.Latitude << "\n";
    break;

  case 1:

    if (tp.valid() && (tp.getActiveState() != OrderedTaskPoint::BEFORE_ACTIVE)) {
      assert(tp.get_previous());
      assert(tp.get_next());
      // note in general this will only change if 
      // prev max or target changes

      AATIsolineSegment seg(tp, projection);
      fixed tdist = tp.get_previous()->get_location_remaining().distance(
        tp.get_location_min());
      fixed rdist = tp.get_previous()->get_location_remaining().distance(
        tp.get_location_target());

      bool filter_backtrack = true;
      if (seg.valid()) {
        for (double t = 0.0; t<=1.0; t+= 1.0/20) {
          GeoPoint ga = seg.parametric(fixed(t));
          fixed dthis = tp.get_previous()->get_location_remaining().distance(ga);
          if (!filter_backtrack 
              || (dthis>=tdist)
              || (dthis>=rdist)) {
            /// @todo unless double dist is better than current
            f << ga.Longitude << " " << ga.Latitude << "\n";
          }
        }
      } else {
        GeoPoint ga = seg.parametric(fixed_zero);
        f << ga.Longitude << " " << ga.Latitude << "\n";
      }
      f << "\n";

    }
    break;
  }
}


void 
PrintHelper::orderedtaskpoint_print(std::ostream& f, 
                                    const OrderedTaskPoint& tp,
                                    const AIRCRAFT_STATE& state,
                                    const int item) 
{
  if (item==0) {
    sampledtaskpoint_print(f,tp,state);
    orderedtaskpoint_print_boundary(f,tp,state);
    f << "# Entered " << tp.get_state_entered().Time << "\n";
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
                                             const AIRCRAFT_STATE &state) 
{
  f << "#   Boundary points\n";
  for (double t=0; t<= 1.0; t+= 0.05) {
    GeoPoint loc = tp.get_boundary_parametric(fixed(t));
    f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  }
  GeoPoint loc = tp.get_boundary_parametric(fixed_zero);
  f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  f << "\n";
}


void 
PrintHelper::sampledtaskpoint_print(std::ostream& f, const SampledTaskPoint& tp,
                                    const AIRCRAFT_STATE &state) 
{
  taskpoint_print(f,tp,state);
}


void 
PrintHelper::sampledtaskpoint_print_samples(std::ostream& f,
                                            const SampledTaskPoint& tp,
                                            const AIRCRAFT_STATE &state) 
{
  const unsigned n= tp.get_search_points().size();
  f << "#   Search points\n";
  if (tp.has_entered()) {
    for (unsigned i=0; i<n; i++) {
      const GeoPoint loc = tp.get_search_points()[i].get_location();
      f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
    }
  }
  f << "\n";
}


void 
PrintHelper::taskpoint_print(std::ostream& f, const TaskPoint& tp,
                             const AIRCRAFT_STATE &state) 
{
  f << "# Task point \n";
  f << "#   Location " << tp.get_location().Longitude << "," <<
    tp.get_location().Latitude << "\n";
}


void
PrintHelper::abstracttask_print(AbstractTask& task, const AIRCRAFT_STATE &state) 
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
       << " " << task.stats.total.solution_remaining.AltitudeDifference
       << "\n";
    f6.flush();
  } else {
    f6 << "\n";
    f6.flush();
  }
}


void 
PrintHelper::gototask_print(GotoTask& task, const AIRCRAFT_STATE &state) 
{
  abstracttask_print(task, state);
  if (task.tp) {
    std::ofstream f1("results/res-goto.txt");
    taskpoint_print(f1,*task.tp,state);
  }
}

void 
PrintHelper::orderedtask_print(OrderedTask& task, const AIRCRAFT_STATE &state) 
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
    f2 <<  tp->get_location_max().Longitude << " " 
       <<  tp->get_location_max().Latitude << "\n";
  }

  std::ofstream f3("results/res-min.txt");
  f3 << "#### Min task\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    OrderedTaskPoint *tp = task.task_points[i];
    f3 <<  tp->get_location_min().Longitude << " " 
       <<  tp->get_location_min().Latitude << "\n";
  }

  std::ofstream f4("results/res-rem.txt");
  f4 << "#### Remaining task\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    OrderedTaskPoint *tp = task.task_points[i];
    f4 <<  tp->get_location_remaining().Longitude << " " 
       <<  tp->get_location_remaining().Latitude << "\n";
  }
}


void PrintHelper::aborttask_print(AbortTask& task, const AIRCRAFT_STATE &state)
{
  abstracttask_print(task, state);

  std::ofstream f1("results/res-abort-task.txt");
  f1 << "#### Task points\n";
  for (unsigned i=0; i<task.task_points.size(); i++) {
    GeoPoint l = task.task_points[i].first->get_location();
    f1 << "## point " << i << " ###################\n";
    if (i==task.activeTaskPoint) {
      f1 << state.Location.Longitude << " " << state.Location.Latitude << "\n";
    }
    f1 << l.Longitude << " " << l.Latitude << "\n";
    f1 << "\n";
  }
}


void PrintHelper::taskmanager_print(TaskManager& task, const AIRCRAFT_STATE &state)
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

  trace_print(task.trace_full, state.Location);

  contestmanager_print(task.contest_manager);

  std::ofstream fs("results/res-stats-common.txt");
  const ContestResult& score = task.get_contest_stats().get_contest_result();
  fs << "#   score " << score.score << "\n";
  fs << "#   distance " << score.distance/fixed(1000) << " (km)\n";
  fs << "#   speed " << score.speed*fixed(3.6) << " (kph)\n";
  fs << "#   time " << score.time << " (sec)\n";
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
  if (gl.Solution != GlideResult::RESULT_OK) {
    f << "#     Solution NOT OK\n";
  }
  f << "#    Altitude Difference " << gl.AltitudeDifference << " (m)\n";
  f << "#    Distance            " << gl.Vector.Distance << " (m)\n";
  f << "#    TrackBearing        " << gl.Vector.Bearing << " (deg)\n";
  f << "#    CruiseTrackBearing  " <<  gl.CruiseTrackBearing << " (deg)\n";
  f << "#    VOpt                " <<  gl.VOpt << " (m/s)\n";
  f << "#    HeightClimb         " <<  gl.HeightClimb << " (m)\n";
  f << "#    HeightGlide         " <<  gl.HeightGlide << " (m)\n";
  f << "#    TimeElapsed         " <<  gl.TimeElapsed << " (s)\n";
  f << "#    TimeVirtual         " <<  gl.TimeVirtual << " (s)\n";
  if (positive(gl.TimeElapsed)) {
    f << "#    Vave remaining      " <<  gl.Vector.Distance/gl.TimeElapsed << " (m/s)\n";
  }
  f << "#    EffectiveWindSpeed  " <<  gl.EffectiveWindSpeed << " (m/s)\n";
  f << "#    EffectiveWindAngle  " <<  gl.EffectiveWindAngle << " (deg)\n";
  f << "#    DistanceToFinal     " <<  gl.DistanceToFinal << " (m)\n";
  if (gl.is_final_glide()) {
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
  f << "#  Time started " << es.TimeStarted << " (s)\n";
  f << "#  Time elapsed " << es.TimeElapsed << " (s)\n";
  f << "#  Time remaining " << es.TimeRemaining << " (s)\n";
  f << "#  Time planned " << es.TimePlanned << " (s)\n";
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

#include "Waypoint/Waypoint.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const Waypoint& wp)
{
  f << wp.Location.Longitude << " " << wp.Location.Latitude << "\n";
  return f;
}

#include "Navigation/Flat/FlatBoundingBox.hpp"

/*
void 
FlatBoundingBox::print(std::ostream &f, const TaskProjection &task_projection) const {
  FlatGeoPoint ll(bb_ll.Longitude,bb_ll.Latitude);
  FlatGeoPoint lr(bb_ur.Longitude,bb_ll.Latitude);
  FlatGeoPoint ur(bb_ur.Longitude,bb_ur.Latitude);
  FlatGeoPoint ul(bb_ll.Longitude,bb_ur.Latitude);
  GeoPoint gll = task_projection.unproject(ll);
  GeoPoint glr = task_projection.unproject(lr);
  GeoPoint gur = task_projection.unproject(ur);
  GeoPoint gul = task_projection.unproject(ul);
  
  f << gll.Longitude << " " << gll.Latitude << "\n";
  f << glr.Longitude << " " << glr.Latitude << "\n";
  f << gur.Longitude << " " << gur.Latitude << "\n";
  f << gul.Longitude << " " << gul.Latitude << "\n";
  f << gll.Longitude << " " << gll.Latitude << "\n";
  f << "\n";
}
*/

/*
void
TaskMacCready::print(std::ostream &f, const AIRCRAFT_STATE &aircraft) const
{
  AIRCRAFT_STATE aircraft_start = get_aircraft_start(aircraft);
  AIRCRAFT_STATE aircraft_predict = aircraft;
  aircraft_predict.Altitude = aircraft_start.Altitude;
  f << "#  i alt  min  elev\n";
  f << start-0.5 << " " << aircraft_start.Altitude << " " <<
    minHs[start] << " " <<
    task_points[start]->get_elevation() << "\n";
  for (int i=start; i<=end; i++) {
    aircraft_predict.Altitude -= gs[i].HeightGlide;
    f << i << " " << aircraft_predict.Altitude << " " << minHs[i]
      << " " << task_points[i]->get_elevation() << "\n";
  }
  f << "\n";
}
*/

#include "Airspace/AirspaceCircle.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceCircle& as)
{
  f << "# circle\n";
  for (double t=0; t<=360; t+= 30) {
    GeoPoint l = FindLatitudeLongitude(as.m_center, Angle::degrees(fixed(t)), as.m_radius);
    f << l.Longitude << " " << l.Latitude << " " << as.get_base().Altitude << "\n";
  }
  f << "\n";
  for (double t=0; t<=360; t+= 30) {
    GeoPoint l = FindLatitudeLongitude(as.m_center, Angle::degrees(fixed(t)), as.m_radius);
    f << l.Longitude << " " << l.Latitude << " " << as.get_top().Altitude << "\n";
  }
  f << "\n";
  f << "\n";
  return f;
}

#include "Airspace/AirspacePolygon.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspacePolygon& as)
{
  f << "# polygon\n";
  for (std::vector<SearchPoint>::const_iterator v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GeoPoint l = v->get_location();
    f << l.Longitude << " " << l.Latitude << " " << as.get_base().Altitude << "\n";
  }
  f << "\n";
  for (std::vector<SearchPoint>::const_iterator v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GeoPoint l = v->get_location();
    f << l.Longitude << " " << l.Latitude << " " << as.get_top().Altitude << "\n";
  }
  f << "\n";
  f << "\n";

  return f;
}


#include "Airspace/AbstractAirspace.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AbstractAirspace& as)
{
  switch (as.shape) {
  case AbstractAirspace::CIRCLE:
    f << (const AirspaceCircle &)as;
    break;

  case AbstractAirspace::POLYGON:
    f << (const AirspacePolygon &)as;
    break;
  }
  return f;
}

#include "Airspace/AirspaceWarning.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceWarning& aw)
{
  AirspaceWarning::AirspaceWarningState state = aw.get_warning_state();
  f << "# warning ";
  switch(state) {
  case AirspaceWarning::WARNING_CLEAR:
    f << "clear\n";
    break;
  case AirspaceWarning::WARNING_TASK:
    f << "task\n";
    break;
  case AirspaceWarning::WARNING_FILTER:
    f << "predicted filter\n";
    break;
  case AirspaceWarning::WARNING_GLIDE:
    f << "predicted glide\n";
    break;
  case AirspaceWarning::WARNING_INSIDE:
    f << "inside\n";
    break;
  };

  const AirspaceInterceptSolution &solution = aw.get_solution();
  f << "# intercept " << solution.location.Longitude << " " << solution.location.Latitude
    << " dist " << solution.distance << " alt " << solution.altitude << " time "
    << solution.elapsed_time << "\n";

  return f;
}


void
PrintHelper::contestmanager_print(const ContestManager& man)  
{
  {
    std::ofstream fs("results/res-olc-trace.txt");
    TracePointVector v;
    man.trace_full.get_trace_points(v);

    for (TracePointVector::const_iterator it = v.begin();
         it != v.end(); ++it) {
      fs << it->get_location().Longitude << " " << it->get_location().Latitude 
         << " " << it->NavAltitude << " " << it->time 
         << "\n";
    }
  }

  {
    std::ofstream fs("results/res-olc-trace_sprint.txt");

    TracePointVector v;
    man.trace_sprint.get_trace_points(v);

    for (TracePointVector::const_iterator it = v.begin();
         it != v.end(); ++it) {
      fs << it->get_location().Longitude << " " << it->get_location().Latitude 
         << " " << it->NavAltitude << " " << it->time 
         << "\n";
    }
  }

  std::ofstream fs("results/res-olc-solution.txt");

  if (man.stats.solution[0].empty()) {
    fs << "# no solution\n";
    return;
  }

  if (positive(man.stats.result[0].time)) {

    for (const TracePoint* it = man.stats.solution[0].begin();
         it != man.stats.solution[0].end(); ++it) {
      fs << it->get_location().Longitude << " " << it->get_location().Latitude 
         << " " << it->NavAltitude << " " << it->time 
         << "\n";
    }
  }
}

void 
PrintHelper::print(const ContestResult& score)
{
  std::cout << "#   score " << score.score << "\n";
  std::cout << "#   distance " << score.distance/fixed(1000) << " (km)\n";
  std::cout << "#   speed " << score.speed*fixed(3.6) << " (kph)\n";
  std::cout << "#   time " << score.time << " (sec)\n";
}


static void
print_tpv(const TracePointVector& vec, std::ofstream& fs)
{
  unsigned last_time = 0;
  for (TracePointVector::const_iterator it = vec.begin(); it != vec.end();
       ++it) {
    if (it->last_time != last_time) {
      fs << "\n";
    }
    fs << it->time 
       << " " << it->get_location().Longitude 
       << " " << it->get_location().Latitude
       << " " << it->NavAltitude
       << " " << it->last_time
       << " " << it->Vario
       << "\n";
    last_time = it->time;
  }
}

void
PrintHelper::trace_print(const Trace& trace, const GeoPoint &loc)
{
  std::ofstream fs("results/res-trace.txt");

  TracePointVector vec = trace.find_within_range(loc, fixed(10000),
                                                 0);
  print_tpv(vec, fs);

  std::ofstream ft("results/res-trace-thin.txt");
  vec = trace.find_within_range(loc, fixed(10000), 0, fixed(1000));

  print_tpv(vec, ft);
}


#include "Math/Angle.hpp"

std::ostream& operator<< (std::ostream& o, const Angle& a)
{
  o << a.value_degrees();
  return o;
} 


void write_point(const SearchPoint& sp, const FlatGeoPoint& p, const char* name)
{
  printf("%g %g %d %d # %s\n",
         (double)sp.get_location().Longitude.value_degrees(),
         (double)sp.get_location().Latitude.value_degrees(),
         p.Longitude,
         p.Latitude,
         name);
  fflush(stdout);
}

void write_spv (const SearchPointVector& spv)
{
  for (std::vector<SearchPoint>::const_iterator v = spv.begin();
       v != spv.end(); ++v) {
    write_point(*v, v->get_flatLocation(), "spv");
  }
  printf("spv\n");
  fflush(stdout);
}

void write_border (const AbstractAirspace& as)
{
  const SearchPointVector& spv = as.get_points();
  for (std::vector<SearchPoint>::const_iterator v = spv.begin();
       v != spv.end(); ++v) {
    write_point(*v, v->get_flatLocation(), "polygon");
  }
  printf("polygon\n");
  write_spv(as.get_clearance());
  fflush(stdout);
}

#include "Route/AirspaceRoute.hpp"

void PrintHelper::print_route(RoutePlanner& r)
{
  for (Route::const_iterator i = r.solution_route.begin();
       i!= r.solution_route.end(); ++i) {
    printf("%.6g %.6g %d # solution\n",
           (double)i->Longitude.value_degrees(),
           (double)i->Latitude.value_degrees(),
           0);
  }
  printf("# solution\n");
  for (Route::const_iterator i = r.solution_route.begin();
       i!= r.solution_route.end(); ++i) {
    printf("%.6g %.6g %d # solution\n",
           (double)i->Longitude.value_degrees(),
           (double)i->Latitude.value_degrees(),
           i->altitude);
  }
  printf("# solution\n");
  printf("# solution\n");
  printf("# stats:\n");
  printf("#   dijkstra links %d\n", (int)r.count_dij);
  printf("#   unique links %d\n", (int)r.count_unique);
  printf("#   airspace queries %d\n", (int)r.count_airspace);
  printf("#   terrain queries %d\n", (int)r.count_terrain);
  printf("#   supressed %d\n", (int)r.count_supressed);
}

#include "Route/ReachFan.hpp"

void
PrintHelper::print_reach_tree(const RoutePlanner& r)
{
  print(r.reach);
}

void
PrintHelper::print(const ReachFan& r)
{
  print(r.root);
}

void
PrintHelper::print(const FlatTriangleFanTree& r) {
  print((const FlatTriangleFan&)r, r.depth);

  for (FlatTriangleFanTree::LeafVector::const_iterator it = r.children.begin();
       it != r.children.end(); ++it) {
    print(*it);
  }
};

void
PrintHelper::print(const FlatTriangleFan& r, const unsigned depth) {
  if (r.vs.size()<3)
    return;

  if (depth) {
    printf("%d %d # fcorner\n", r.vs[0].Longitude, r.vs[0].Latitude);
  }

  for (FlatTriangleFan::VertexVector::const_iterator it = r.vs.begin();
       it != r.vs.end(); ++it) {
    const FlatGeoPoint p = (*it);
    printf("%d %d # ftri\n", p.Longitude, p.Latitude);
  }
  printf("%d %d # ftri\n", r.vs[0].Longitude, r.vs[0].Latitude);
  printf("# ftri\n");
}

