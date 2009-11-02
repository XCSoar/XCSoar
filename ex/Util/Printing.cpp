#include "Math/Earth.hpp"
#include "Navigation/TaskProjection.hpp"

#ifdef DO_PRINT
#include <fstream>

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

#include "GlideSolvers/GlideResult.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const GLIDE_RESULT& gl)
{
  if (gl.Solution != GLIDE_RESULT::RESULT_OK) {
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
  f << "#  Planned: \n";
  f << es.planned;
  f << es.solution_planned;
  f << "#  Travelled: \n";
  f << es.travelled;
  f << es.solution_travelled;
  return f;
}

#include "Navigation/Waypoint.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const Waypoint& wp)
{
  f << wp.Location.Longitude << " " << wp.Location.Latitude << "\n";
  return f;
}

#include "Airspace/FlatBoundingBox.hpp"

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
    tps[start]->getElevation() << "\n";
  for (int i=start; i<=end; i++) {
    aircraft_predict.Altitude -= gs[i].HeightGlide;
    f << i << " " << aircraft_predict.Altitude << " " << minHs[i]
      << " " << tps[i]->getElevation() << "\n";
  }
  f << "\n";
}
*/

#include "Airspace/Airspace.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const Airspace& ts) 
{
  if (ts.pimpl_airspace) {
    f << *(ts.pimpl_airspace);
  } 
  return f;
}

#include "Airspace/AirspaceCircle.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceCircle& as)
{
  f << "# circle\n";
  for (double t=0; t<=360; t+= 30) {
    GEOPOINT l;
    FindLatitudeLongitude(as.center, t, as.radius, &l);
    f << l.Longitude << " " << l.Latitude << "\n";
  }
  f << "\n";
  return f;
}

#include "Airspace/AirspacePolygon.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspacePolygon& as)
{
  f << "# polygon\n";
  for (std::vector<SearchPoint>::const_iterator v = as.border.begin();
       v != as.border.end(); v++) {
    GEOPOINT l = v->getLocation();
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
  f << "#   Location " << getLocation().Longitude << "," <<
    getLocation().Latitude << "\n";
}


#include "Task/Tasks/AbstractTask.hpp"

void
AbstractTask::print(const AIRCRAFT_STATE &state)
{
  std::ofstream fs("res-stats-all.txt");
  fs << stats;

  static std::ofstream f6("res-stats.txt");
  static bool first = true;

  if (first) {
    first = false;
    f6 << "# Time atp mc_best d_tot_rem_eff d_tot_rem ceff v_tot_rem v_tot_rem_inc v_tot_eff v_tot_eff_inc\n";
  }
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
     << "\n";
  f6.flush();
}

#include "Task/Tasks/GotoTask.hpp"

void 
GotoTask::print(const AIRCRAFT_STATE &state)
{
  AbstractTask::print(state);
  if (tp) {
    std::ofstream f1("res-goto.txt");
    tp->print(f1,state);
  }
}

#include "Task/Tasks/OrderedTask.hpp"

void OrderedTask::print(const AIRCRAFT_STATE &state) 
{
  AbstractTask::print(state);

  std::ofstream fi("res-isolines.txt");
  for (unsigned i=0; i<tps.size(); i++) {
    fi << "## point " << i << "\n";
    tps[i]->print(fi,state,1);
    fi << "\n";
  }

  std::ofstream f1("res-task.txt");

  f1 << "#### Task points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    f1 << "## point " << i << " ###################\n";
    tps[i]->print(f1,state,0);
    f1 << "\n";
  }

  std::ofstream f5("res-ssample.txt");
  f5 << "#### Task sampled points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    f5 << "## point " << i << "\n";
    tps[i]->print_samples(f5,state);
    f5 << "\n";
  }

  std::ofstream f2("res-max.txt");
  f2 << "#### Max task\n";
  for (unsigned i=0; i<tps.size(); i++) {
    OrderedTaskPoint *tp = tps[i];
    f2 <<  tp->getMaxLocation().Longitude << " " 
       <<  tp->getMaxLocation().Latitude << "\n";
  }

  std::ofstream f3("res-min.txt");
  f3 << "#### Min task\n";
  for (unsigned i=0; i<tps.size(); i++) {
    OrderedTaskPoint *tp = tps[i];
    f3 <<  tp->getMinLocation().Longitude << " " 
       <<  tp->getMinLocation().Latitude << "\n";
  }

  std::ofstream f4("res-rem.txt");
  f4 << "#### Remaining task\n";
  for (unsigned i=0; i<tps.size(); i++) {
    OrderedTaskPoint *tp = tps[i];
    f4 <<  tp->get_reference_remaining().Longitude << " " 
       <<  tp->get_reference_remaining().Latitude << "\n";
  }

}

#include "Task/Tasks/AbortTask.hpp"

void AbortTask::print(const AIRCRAFT_STATE &state)
{
  AbstractTask::print(state);

  std::ofstream f1("res-abort-task.txt");
  f1 << "#### Task points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    GEOPOINT l = tps[i]->getLocation();
    f1 << "## point " << i << " ###################\n";
    if (i==activeTaskPoint) {
      f1 << state.Location.Longitude << " " << state.Location.Latitude << "\n";
    }
    f1 << l.Longitude << " " << l.Latitude << "\n";
    f1 << "\n";
  }
}

#include "Task/TaskManager.hpp"

void TaskManager::print(const AIRCRAFT_STATE &state)
{
  if (active_task) 
    return active_task->print(state);
}

#include "Task/Tasks/BaseTask/AATPoint.hpp"
#include "Task/Tasks/BaseTask/AATIsolineSegment.hpp"

void AATPoint::print(std::ostream& f, const AIRCRAFT_STATE& state,
                     const int item) const
{
  switch(item) {
  case 0:
    OrderedTaskPoint::print(f, state, item);
    f << "#   Target " << TargetLocation.Longitude << "," 
      << TargetLocation.Latitude << "\n";
    break;

  case 1:

    if (getActiveState() != BEFORE_ACTIVE) {

      // note in general this will only change if 
      // prev max or target changes

      AATIsolineSegment seg(*this);
      double tdist = ::Distance(get_previous()->get_reference_remaining(),
                                getMinLocation());
      double rdist = ::Distance(get_previous()->get_reference_remaining(),
                                getTargetLocation());

      bool filter_backtrack = true;
      if (seg.valid()) {
        for (double t = 0.0; t<=1.0; t+= 1.0/20) {
          GEOPOINT ga = seg.parametric(t);
          double dthis = ::Distance(get_previous()->get_reference_remaining(),
                                    ga);
          if (!filter_backtrack 
              || (dthis>=tdist)
              || (dthis>=rdist)) {
            // TODO: unless double dist is better than current
            f << ga.Longitude << " " << ga.Latitude << "\n";
          }
        }
      } else {
        GEOPOINT ga = seg.parametric(0.0);
        f << ga.Longitude << " " << ga.Latitude << "\n";
      }
      f << "\n";

    }
    break;
  }
}

void 
AATPoint::print_boundary(std::ostream& f,
  const AIRCRAFT_STATE &state) const
{
  const unsigned n= get_boundary_points().size();
  f << "#   Boundary points\n";
  const double mind = double_leg_distance(state.Location);
  for (unsigned i=0; i<n; i++) {
    const GEOPOINT loc = get_boundary_points()[i].getLocation();

    // TODO: this is broken
    if (1 || double_leg_distance(loc)>mind) {
      f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
    } else {
      f << "     " << state.Location.Longitude << " " << state.Location.Latitude << "\n";
    }
  }
  f << "\n";
}

#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"

void 
OrderedTaskPoint::print(std::ostream& f, const AIRCRAFT_STATE& state,
                        const int item) const
{
  if (item==0) {
    SampledTaskPoint::print(f,state);
    f << "# Entered " << get_state_entered().Time << "\n";
    f << "# Bearing travelled " << vector_travelled.Bearing << "\n";
    f << "# Distance travelled " << vector_travelled.Distance << "\n";
    f << "# Bearing remaining " << vector_remaining.Bearing << "\n";
    f << "# Distance remaining " << vector_remaining.Distance << "\n";
    f << "# Bearing planned " << vector_planned.Bearing << "\n";
    f << "# Distance planned " << vector_planned.Distance << "\n";
  }
}


#include "Task/Tasks/BaseTask/SampledTaskPoint.hpp"

void 
SampledTaskPoint::print_boundary(std::ostream& f, const AIRCRAFT_STATE &state) const
{
  f << "#   Boundary points\n";
  const unsigned n= get_boundary_points().size();
  for (unsigned i=0; i<n; i++) {
    const GEOPOINT loc = get_boundary_points()[i].getLocation();
    f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  }
  f << "\n";
}


void 
SampledTaskPoint::print(std::ostream& f, const AIRCRAFT_STATE &state) const
{
  TaskPoint::print(f,state);
  print_boundary(f, state);
}

void 
SampledTaskPoint::print_samples(std::ostream& f,
  const AIRCRAFT_STATE &state) 
{
  const unsigned n= get_search_points().size();
  f << "#   Search points\n";
  for (unsigned i=0; i<n; i++) {
    const GEOPOINT loc = get_search_points()[i].getLocation();
    f << "     " << loc.Longitude << " " << loc.Latitude << "\n";
  }
  f << "\n";
}

#endif
