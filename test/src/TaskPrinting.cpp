/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Engine/Task/TaskManager.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Task/Points/SampledTaskPoint.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Ordered/AATIsolineSegment.hpp"
#include "Engine/Task/Unordered/GotoTask.hpp"
#include "Engine/Task/Unordered/AbortTask.hpp"
#include "Engine/Task/Stats/CommonStats.hpp"
#include "Engine/Task/ObservationZones/Boundary.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Geo/Math.hpp"
#include "OS/FileUtil.hpp"

#include <fstream>

static std::ostream &
operator<<(std::ostream &f, const GlideResult &gl)
{
  if (gl.validity != GlideResult::Validity::OK) {
    f << "#     Solution NOT OK\n";
  }
  f << "#    Altitude Difference " << gl.altitude_difference << " (m)\n";
  f << "#    Distance            " << gl.vector.distance << " (m)\n";
  f << "#    TrackBearing        " << gl.vector.bearing << " (deg)\n";
  f << "#    CruiseTrackBearing  " <<  gl.cruise_track_bearing << " (deg)\n";
  f << "#    VOpt                " <<  gl.v_opt << " (m/s)\n";
  f << "#    HeightClimb         " <<  gl.height_climb << " (m)\n";
  f << "#    HeightGlide         " <<  gl.height_glide << " (m)\n";
  f << "#    TimeElapsed         " <<  gl.time_elapsed << " (s)\n";
  f << "#    TimeVirtual         " <<  gl.time_virtual << " (s)\n";
  if (gl.time_elapsed > 0) {
    f << "#    Vave remaining      " <<  gl.vector.distance/gl.time_elapsed << " (m/s)\n";
  }
  f << "#    EffectiveWindSpeed  " <<  gl.effective_wind_speed << " (m/s)\n";
  f << "#    EffectiveWindAngle  " <<  gl.effective_wind_angle << " (deg)\n";
  if (gl.IsFinalGlide()) {
    f << "#    On final glide\n";
  }
  return f;
}

static std::ostream &
operator<<(std::ostream &f, const DistanceStat &ds)
{
  if (ds.IsDefined()) {
    f << "#    Distance " << ds.GetDistance() << " (m)\n";
    f << "#    Speed " << ds.GetSpeed() << " (m/s)\n";
    f << "#    Speed incremental " << ds.GetSpeedIncremental() << " (m/s)\n";
  } else {
    f << "#    (undefined)\n";
  }
  return f;
}

static std::ostream &
operator<<(std::ostream &f, const ElementStat &es)
{
  f << "#  Time started " << es.time_started << " (s)\n";
  f << "#  Time elapsed " << es.time_elapsed << " (s)\n";
  f << "#  Time remaining " << es.time_remaining_now << " (s)\n";
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

#include "Task/Stats/TaskStats.hpp"

static std::ostream &
operator<<(std::ostream &f, const TaskStats &ts)
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
    f << "#   Target " << tp.GetTargetLocation().longitude << ","
      << tp.GetTargetLocation().latitude << "\n";
    break;

  case 1:

    if (tp.valid() && (tp.GetActiveState() != OrderedTaskPoint::BEFORE_ACTIVE)) {
      assert(tp.GetPrevious());
      assert(tp.GetNext());
      // note in general this will only change if 
      // prev max or target changes

      AATIsolineSegment seg(tp, projection);
      auto tdist = tp.GetPrevious()->GetLocationRemaining().Distance(tp.GetLocationMin());
      auto rdist = tp.GetPrevious()->GetLocationRemaining().Distance(tp.GetTargetLocation());

      bool filter_backtrack = true;
      if (seg.IsValid()) {
        for (double t = 0.0; t<=1.0; t+= 1.0/20) {
          GeoPoint ga = seg.Parametric(t);
          double dthis = tp.GetPrevious()->GetLocationRemaining().Distance(ga);
          if (!filter_backtrack 
              || (dthis>=tdist)
              || (dthis>=rdist)) {
            /// @todo unless double dist is better than current
            f << ga.longitude << " " << ga.latitude << "\n";
          }
        }
      } else {
        GeoPoint ga = seg.Parametric(0);
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
    taskpoint_print(f,tp,state);
    orderedtaskpoint_print_boundary(f,tp,state);
    f << "# Entered " << tp.GetEnteredState().time << "\n";
    f << "# Bearing travelled " << tp.GetVectorTravelled().bearing << "\n";
    f << "# Distance travelled " << tp.GetVectorTravelled().distance << "\n";
    f << "# Bearing remaining " << tp.GetVectorRemaining(state.location).bearing << "\n";
    f << "# Distance remaining " << tp.GetVectorRemaining(state.location).distance << "\n";
    f << "# Bearing planned " << tp.GetVectorPlanned().bearing << "\n";
    f << "# Distance planned " << tp.GetVectorPlanned().distance << "\n";
  }
}


void 
PrintHelper::orderedtaskpoint_print_boundary(std::ostream& f, 
                                             const OrderedTaskPoint& tp,
                                             const AircraftState &state) 
{
  f << "#   Boundary points\n";
  for (const auto &i : tp.GetBoundary())
    f << "     " << i.longitude << " " << i.latitude << "\n";
  f << "\n";
}

void 
PrintHelper::sampledtaskpoint_print_samples(std::ostream& f,
                                            const ScoredTaskPoint &tp,
                                            const AircraftState &state) 
{
  const unsigned n= tp.GetSearchPoints().size();
  f << "#   Search points\n";
  if (tp.HasEntered()) {
    for (unsigned i=0; i<n; i++) {
      const GeoPoint loc = tp.GetSearchPoints()[i].GetLocation();
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
PrintHelper::abstracttask_print(const AbstractTask &task,
                                const AircraftState &state)
{
  Directory::Create(Path(_T("output/results")));
  std::ofstream fs("output/results/res-stats-all.txt");

  const auto &stats = task.GetStats();

  if (!stats.task_valid)
    return;

  fs << stats;

  static std::ofstream f6("output/results/res-stats.txt");
  static bool first = true;

  if (first) {
    first = false;
    f6 << "# Time atp mc_best d_tot_rem_eff d_tot_rem ceff v_tot_rem v_tot_rem_inc v_tot_eff v_tot_eff_inc task_vario effective_mc v_pirker alt_diff\n";
  }

  f6 << " " << task.GetActiveTaskPointIndex()
     << " " << stats.mc_best
     << " " << (stats.total.remaining_effective.IsDefined()
                ? stats.total.remaining_effective.GetDistance()
                : 0)
     << " " << stats.total.remaining.GetDistance()
     << " " << stats.cruise_efficiency
     << " " << stats.total.remaining.GetSpeed()
     << " " << stats.total.remaining.GetSpeedIncremental()
     << " " << (stats.total.remaining_effective.IsDefined()
                ? stats.total.remaining_effective.GetSpeed()
                : 0)
     << " " << (stats.total.remaining_effective.IsDefined()
                ? stats.total.remaining_effective.GetSpeedIncremental()
                : 0)
     << " " << stats.total.vario.get_value()
     << " " << stats.effective_mc
     << " " << (stats.task_valid
                ? stats.inst_speed_slow
                : -1)
     << " " << stats.total.solution_remaining.altitude_difference
     << "\n";
  f6.flush();
}


void 
PrintHelper::gototask_print(const GotoTask &task,
                            const AircraftState &state)
{
  abstracttask_print(task, state);

  const TaskWaypoint *tp = task.GetActiveTaskPoint();
  if (tp != nullptr) {
    std::ofstream f1("output/results/res-goto.txt");
    taskpoint_print(f1, *tp, state);
  }
}

void 
PrintHelper::orderedtask_print(const OrderedTask &task,
                               const AircraftState &state)
{
  abstracttask_print(task, state);
  if (!task.CheckTask())
    return;

  std::ofstream fi("output/results/res-isolines.txt");
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    const OrderedTaskPoint &tp = task.GetPoint(i);
    fi << "## point " << i << "\n";
    if (tp.GetType() == TaskPointType::AAT) {
      aatpoint_print(fi, (const AATPoint &)tp, state,
                     task.GetTaskProjection(), 1);
    } else {
      orderedtaskpoint_print(fi, tp, state, 1);
    }
    fi << "\n";
  }

  std::ofstream f1("output/results/res-task.txt");

  f1 << "#### Task points\n";
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    f1 << "## point " << i << " ###################\n";
    const OrderedTaskPoint &tp = task.GetPoint(i);
    if (tp.GetType() == TaskPointType::AAT) {
      aatpoint_print(f1, (const AATPoint &)tp, state,
                     task.GetTaskProjection(), 0);
    } else {
      orderedtaskpoint_print(f1, tp, state, 0);
    }
    f1 << "\n";
  }

  std::ofstream f5("output/results/res-ssample.txt");
  f5 << "#### Task sampled points\n";
  for (unsigned i =0 ; i < task.TaskSize(); ++i) {
    const OrderedTaskPoint &tp = task.GetPoint(i);
    f5 << "## point " << i << "\n";
    sampledtaskpoint_print_samples(f5, tp, state);
    f5 << "\n";
  }

  std::ofstream f2("output/results/res-max.txt");
  f2 << "#### Max task\n";
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    const OrderedTaskPoint &tp = task.GetPoint(i);
    f2 << tp.GetLocationMax().longitude << " "
       << tp.GetLocationMax().latitude << "\n";
  }

  std::ofstream f3("output/results/res-min.txt");
  f3 << "#### Min task\n";
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    const OrderedTaskPoint &tp = task.GetPoint(i);
    f3 << tp.GetLocationMin().longitude << " "
       << tp.GetLocationMin().latitude << "\n";
  }

  std::ofstream f4("output/results/res-rem.txt");
  f4 << "#### Remaining task\n";
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    const OrderedTaskPoint &tp = task.GetPoint(i);
    f4 << tp.GetLocationRemaining().longitude << " "
       << tp.GetLocationRemaining().latitude << "\n";
  }
}


void
PrintHelper::aborttask_print(const AbortTask &task, const AircraftState &state)
{
  abstracttask_print(task, state);

  std::ofstream f1("output/results/res-abort-task.txt");
  f1 << "#### Task points\n";
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    GeoPoint l = task.GetAlternate(i).GetLocation();
    f1 << "## point " << i << " ###################\n";
    if (i == task.GetActiveTaskPointIndex()) {
      f1 << state.location.longitude << " " << state.location.latitude << "\n";
    }
    f1 << l.longitude << " " << l.latitude << "\n";
    f1 << "\n";
  }
}


void
PrintHelper::taskmanager_print(const TaskManager &task,
                               const AircraftState &state)
{
  switch (task.GetMode()) {
  case TaskType::NONE:
    break;

  case TaskType::ABORT:
    aborttask_print(*(const AbortTask *)task.GetActiveTask(), state);
    break;

  case TaskType::GOTO:
    gototask_print(*(const GotoTask *)task.GetActiveTask(), state);
    break;

  case TaskType::ORDERED:
    orderedtask_print(task.GetOrderedTask(), state);
    break;
  }
}

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
