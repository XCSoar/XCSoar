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

#ifdef DO_PRINT
#include <fstream>

#include "Task/TaskManager.hpp"

void TaskManager::print(const AIRCRAFT_STATE &state)
{
  if (active_task) 
    active_task->print(state);

  trace.print(state.Location);

  std::ofstream fs("results/res-stats-common.txt");
  fs << common_stats;
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

  FLAT_GEOPOINT pll, pur;
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
  if (gl.TimeElapsed>0) {
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
                          const CommonStats& ts)
{
  f << "#### Common Stats\n";
  f << "# olc dist " << ts.distance_olc << " (m)\n";
  f << "# olc time " << ts.time_olc << " (s)\n";
  f << "# olc speed " << ts.speed_olc << " (m/s)\n";
  return f;
}


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
  FLAT_GEOPOINT ll(bb_ll.Longitude,bb_ll.Latitude);
  FLAT_GEOPOINT lr(bb_ur.Longitude,bb_ll.Latitude);
  FLAT_GEOPOINT ur(bb_ur.Longitude,bb_ur.Latitude);
  FLAT_GEOPOINT ul(bb_ll.Longitude,bb_ur.Latitude);
  GEOPOINT gll = task_projection.unproject(ll);
  GEOPOINT glr = task_projection.unproject(lr);
  GEOPOINT gur = task_projection.unproject(ur);
  GEOPOINT gul = task_projection.unproject(ul);
  
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
    tps[start]->get_elevation() << "\n";
  for (int i=start; i<=end; i++) {
    aircraft_predict.Altitude -= gs[i].HeightGlide;
    f << i << " " << aircraft_predict.Altitude << " " << minHs[i]
      << " " << tps[i]->get_elevation() << "\n";
  }
  f << "\n";
}
*/

#include "Airspace/AirspaceCircle.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceCircle& as)
{
  f << "# circle " << as.get_base_altitude() << " " << as.get_top_altitude() << "\n";
  for (double t=0; t<=360; t+= 30) {
    GEOPOINT l;
    FindLatitudeLongitude(as.m_center, fixed(t), as.m_radius, &l);
    f << l.Longitude << " " << l.Latitude << "\n";
  }
  f << "\n";
  return f;
}

#include "Airspace/AirspacePolygon.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspacePolygon& as)
{
  f << "# polygon " << as.get_base_altitude() << " " << as.get_top_altitude() << "\n";
  for (std::vector<SearchPoint>::const_iterator v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GEOPOINT l = v->get_location();
    f << l.Longitude << " " << l.Latitude << "\n";
  }
  f << "\n";
  return f;
}


#include "Airspace/AbstractAirspace.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AbstractAirspace& as)
{
  if (const AirspaceCircle* ac = dynamic_cast<const AirspaceCircle*>(&as)) {
    f << *ac;
  }
  if (const AirspacePolygon* ac = dynamic_cast<const AirspacePolygon*>(&as)) {
    f << *ac;
  }
  return f;
}

///////////////////////////////////////////////////////////////////////////

#include "Task/Tasks/BaseTask/TaskPoint.hpp"

void 
TaskPoint::print(std::ostream& f, const AIRCRAFT_STATE &state) const
{
  f << "# Task point \n";
  f << "#   Location " << get_location().Longitude << "," <<
    get_location().Latitude << "\n";
}


#include "Task/Tasks/AbstractTask.hpp"

void
AbstractTask::print(const AIRCRAFT_STATE &state)
{
  std::ofstream fs("results/res-stats-all.txt");
  fs << stats;

  static std::ofstream f6("results/res-stats.txt");
  static bool first = true;

  if (first) {
    first = false;
    f6 << "# Time atp mc_best d_tot_rem_eff d_tot_rem ceff v_tot_rem v_tot_rem_inc v_tot_eff v_tot_eff_inc task_vario effective_mc\n";
  }

  if (stats.Time>0) {
    f6 << stats.Time
       << " " << activeTaskPoint
       << " " << stats.mc_best
       << " " << stats.total.remaining_effective.get_distance()
       << " " << stats.total.remaining.get_distance() 
       << " " << stats.cruise_efficiency 
       << " " << stats.total.remaining.get_speed() 
       << " " << stats.total.remaining.get_speed_incremental() 
       << " " << stats.total.remaining_effective.get_speed() 
       << " " << stats.total.remaining_effective.get_speed_incremental() 
       << " " << stats.total.vario.get_value() 
       << " " << stats.effective_mc
       << "\n";
    f6.flush();
  } else {
    f6 << "\n";
    f6.flush();
  }
}

#include "Task/Tasks/GotoTask.hpp"

void 
GotoTask::print(const AIRCRAFT_STATE &state)
{
  AbstractTask::print(state);
  if (tp) {
    std::ofstream f1("results/res-goto.txt");
    tp->print(f1,state);
  }
}

#include "Task/Tasks/OrderedTask.hpp"

void OrderedTask::print(const AIRCRAFT_STATE &state) 
{
  AbstractTask::print(state);

  std::ofstream fi("results/res-isolines.txt");
  for (unsigned i=0; i<tps.size(); i++) {
    fi << "## point " << i << "\n";
    tps[i]->print(fi,state,1);
    fi << "\n";
  }

  std::ofstream f1("results/res-task.txt");

  f1 << "#### Task points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    f1 << "## point " << i << " ###################\n";
    tps[i]->print(f1,state,0);
    f1 << "\n";
  }

  std::ofstream f5("results/res-ssample.txt");
  f5 << "#### Task sampled points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    f5 << "## point " << i << "\n";
    tps[i]->print_samples(f5,state);
    f5 << "\n";
  }

  std::ofstream f2("results/res-max.txt");
  f2 << "#### Max task\n";
  for (unsigned i=0; i<tps.size(); i++) {
    OrderedTaskPoint *tp = tps[i];
    f2 <<  tp->get_location_max().Longitude << " " 
       <<  tp->get_location_max().Latitude << "\n";
  }

  std::ofstream f3("results/res-min.txt");
  f3 << "#### Min task\n";
  for (unsigned i=0; i<tps.size(); i++) {
    OrderedTaskPoint *tp = tps[i];
    f3 <<  tp->get_location_min().Longitude << " " 
       <<  tp->get_location_min().Latitude << "\n";
  }

  std::ofstream f4("results/res-rem.txt");
  f4 << "#### Remaining task\n";
  for (unsigned i=0; i<tps.size(); i++) {
    OrderedTaskPoint *tp = tps[i];
    f4 <<  tp->get_location_remaining().Longitude << " " 
       <<  tp->get_location_remaining().Latitude << "\n";
  }

}

#include "Task/Tasks/AbortTask.hpp"

void AbortTask::print(const AIRCRAFT_STATE &state)
{
  AbstractTask::print(state);

  std::ofstream f1("results/res-abort-task.txt");
  f1 << "#### Task points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    GEOPOINT l = tps[i]->get_location();
    f1 << "## point " << i << " ###################\n";
    if (i==activeTaskPoint) {
      f1 << state.Location.Longitude << " " << state.Location.Latitude << "\n";
    }
    f1 << l.Longitude << " " << l.Latitude << "\n";
    f1 << "\n";
  }
}


#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"

void AATPoint::print(std::ostream& f, const AIRCRAFT_STATE& state,
                     const int item) const
{
  switch(item) {
  case 0:
    OrderedTaskPoint::print(f, state, item);
    f << "#   Target " << m_target_location.Longitude << "," 
      << m_target_location.Latitude << "\n";
    break;

  case 1:

    if ((get_next()!= NULL) && (getActiveState() != BEFORE_ACTIVE)) {

      // note in general this will only change if 
      // prev max or target changes

      AATIsolineSegment seg(*this);
      double tdist = ::Distance(get_previous()->get_location_remaining(),
                                get_location_min());
      double rdist = ::Distance(get_previous()->get_location_remaining(),
                                get_location_target());

      bool filter_backtrack = true;
      if (seg.valid()) {
        for (double t = 0.0; t<=1.0; t+= 1.0/20) {
          GEOPOINT ga = seg.parametric(fixed(t));
          double dthis = ::Distance(get_previous()->get_location_remaining(),
                                    ga);
          if (!filter_backtrack 
              || (dthis>=tdist)
              || (dthis>=rdist)) {
            /// @todo unless double dist is better than current
            f << ga.Longitude << " " << ga.Latitude << "\n";
          }
        }
      } else {
        GEOPOINT ga = seg.parametric(fixed_zero);
        f << ga.Longitude << " " << ga.Latitude << "\n";
      }
      f << "\n";

    }
    break;
  }
}

#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"

void 
OrderedTaskPoint::print(std::ostream& f, const AIRCRAFT_STATE& state,
                        const int item) const
{
  if (item==0) {
    SampledTaskPoint::print(f,state);
    print_boundary(f, state);
    f << "# Entered " << get_state_entered().Time << "\n";
    f << "# Bearing travelled " << vector_travelled.Bearing << "\n";
    f << "# Distance travelled " << vector_travelled.Distance << "\n";
    f << "# Bearing remaining " << vector_remaining.Bearing << "\n";
    f << "# Distance remaining " << vector_remaining.Distance << "\n";
    f << "# Bearing planned " << vector_planned.Bearing << "\n";
    f << "# Distance planned " << vector_planned.Distance << "\n";
  }
}

void 
OrderedTaskPoint::print_boundary(std::ostream& f, const AIRCRAFT_STATE &state) const
{
  f << "#   Boundary points\n";
  for (double t=0; t<= 1.0; t+= 0.05) {
    GEOPOINT loc = get_boundary_parametric(fixed(t));
    f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  }
  GEOPOINT loc = get_boundary_parametric(fixed_zero);
  f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  f << "\n";
}


#include "Task/Tasks/BaseTask/SampledTaskPoint.hpp"


void 
SampledTaskPoint::print(std::ostream& f, const AIRCRAFT_STATE &state) const
{
  TaskPoint::print(f,state);
}

void 
SampledTaskPoint::print_samples(std::ostream& f,
  const AIRCRAFT_STATE &state) 
{
  const unsigned n= get_search_points().size();
  f << "#   Search points\n";
  for (unsigned i=0; i<n; i++) {
    const GEOPOINT loc = get_search_points()[i].get_location();
    f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  }
  f << "\n";
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

  f << "# intercept " << aw.m_solution.location.Longitude << " " << aw.m_solution.location.Latitude 
    << " dist " << aw.m_solution.distance << " alt " << aw.m_solution.altitude << " time " 
    << aw.m_solution.elapsed_time << "\n";

  return f;
}



#include "Task/Tasks/OnlineContest.hpp"

void
OnlineContest::print() const 
{
  {
    std::ofstream fs("results/res-olc-trace.txt");

    for (TracePointVector::const_iterator it = m_trace_points.begin();
         it != m_trace_points.end(); ++it) {
      fs << it->get_location().Longitude << " " << it->get_location().Latitude 
         << " " << it->altitude << " " << it->time 
         << " " << it->rank
         << "\n";
    }
  }

  if (m_solution.empty()) 
    return;

  {
    std::ofstream fs("results/res-olc-solution.txt");

    for (TracePointVector::const_iterator it = m_solution.begin();
         it != m_solution.end(); ++it) {
      fs << it->get_location().Longitude << " " << it->get_location().Latitude 
         << " " << it->altitude << " " << it->time 
         << "\n";
    }
  }
}


#include "Trace/Trace.hpp"

void print_tpv(const TracePointVector& vec, std::ofstream& fs) 
{
  for (TracePointVector::const_iterator it = vec.begin(); it != vec.end();
       ++it) {
    fs << it->time 
       << " " << it->get_location().Longitude 
       << " " << it->get_location().Latitude
       << " " << it->altitude
       << " " << it->last_time
       << "\n";
  }
}

void
Trace::print(const GEOPOINT &loc) const
{
  std::ofstream fs("results/res-trace.txt");

  TracePointVector vec = find_within_range(loc, fixed(10000),
                                                  0);
  print_tpv(vec, fs);

  std::ofstream ft("results/res-trace-thin.txt");
  thin_trace(vec, loc, fixed(1000));

  print_tpv(vec, ft);
}

#endif
