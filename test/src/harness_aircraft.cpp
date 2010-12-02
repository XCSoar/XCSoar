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

#include "harness_aircraft.hpp"
#include "test_debug.hpp"

#include "Task/Factory/AbstractTaskFactory.hpp"

static void
print_mode(const char* mode)
{
  if (verbose>1) {
    printf("%s",mode);
  }
}

AircraftSim::AircraftSim(int _test_num, 
                         TaskManager& _task_manager,
                         const AutopilotParameters &_parms,
                         bool _goto_target):
  test_num(_test_num),
  heading_filt(fixed(8)),
  goto_target(_goto_target),
  speed_factor(1.0),
  climb_rate(2.0),
  short_flight(false),
  task_manager(_task_manager),
  parms(_parms)
{
  Start();
}

void 
AircraftSim::Stop() 
{
  // nothing to do
}

void
AircraftSim::Start()
{
  w.clear();

  if (task_manager.task_size()<=1) {
    short_flight = true;
    // cheat for non-ordered tasks
    w.push_back(GeoPoint(Angle::degrees(fixed(0.1)), Angle::degrees(fixed(0.1))));
    if (task_manager.task_size()>0) {
      w.push_back(task_manager.random_point_in_task(0, parms.target_noise));
    } else {
      w.push_back(GeoPoint(Angle::degrees(fixed_one), Angle::degrees(fixed_zero)));
    }
  } else {
    for (unsigned i=0; i<task_manager.task_size(); i++) {
      w.push_back(task_manager.random_point_in_task(i, parms.target_noise));
    }
  }
  
  if (task_manager.task_size()>1) {
    // set start location to 200 meters directly behind start
    // (otherwise start may fail)
    Angle brg = w[1].bearing(w[0]);
    state.Location = GeoVector(fixed(200), brg).end_point(w[0]);
  } else {
    state.Location = w[0];
  }
  state.NavAltitude = parms.start_alt;
  state.AirspaceAltitude = parms.start_alt;
  state.Time = fixed_zero;
  state.wind.norm = fixed_zero;
  state.wind.bearing = Angle();
  state.Speed = fixed(16);
  state_last = state;
  state_last.Location = w[0];

  // start with aircraft moving since this isn't a real replay (no time on ground)
  for (unsigned i=0; i<10; i++) {
    state.flying_state_moving(state.Time);
  }
  
  bearing = Angle();
  awp= 0;
  
  acstate = Cruise;
}


GeoPoint AircraftSim::target() {
  if (goto_target && (awp>0)) {
    return task_manager.getActiveTaskPoint()->get_location();
  } else {
    if (awp>0) {
      return w[awp];
    } else {
      return w[awp+1];
    }
  }
}

bool AircraftSim::far() {

  AbstractTaskFactory &fact = task_manager.get_factory();
  bool entered = fact.has_entered(awp);

  if (task_manager.task_size()==1) {
    // cheat for non-ordered tasks
    const ElementStat stat = task_manager.get_stats().current_leg;
    return stat.remaining.get_distance() > fixed(100);
  } else if (task_manager.task_size()==0) {
    return w[1].distance(state.Location)>state.Speed;
  }
  if (goto_target && (awp>0)) {
    const ElementStat stat = task_manager.get_stats().current_leg;
    return stat.remaining.get_distance() > fixed(100) || !entered;
  } else {
    fixed dc = w[awp].distance(state.Location);
    if (!positive(fixed(awp))) {
      return (dc>state.Speed);
    } else {
      return (dc>state.Speed) || !entered;
    }
  }
}

#define fixed_1000 fixed(1000)
#define fixed_20 fixed(20)

fixed AircraftSim::small_rand() {
  return fixed(heading_filt.update(parms.bearing_noise *
                                   (fixed_two * rand() / RAND_MAX) -
                                   parms.bearing_noise));
}

void AircraftSim::update_bearing() {
  const ElementStat stat = task_manager.get_stats().current_leg;
  Angle bct = stat.solution_remaining.CruiseTrackBearing;

  if (goto_target && (awp>0)) {
    bearing = stat.solution_remaining.Vector.Bearing;

    if (enable_bestcruisetrack && (stat.solution_remaining.Vector.Distance>fixed_1000)) {
      bearing = bct;      
    }

  } else {
    bearing = state.Location.bearing(target());
  }

  if (positive(state.wind.norm) && positive(state.TrueAirspeed)) {
    const fixed sintheta = (state.wind.bearing-bearing).sin();
    if (fabs(sintheta)>fixed(0.0001)) {
      bearing +=
        Angle::radians(asin(sintheta * state.wind.norm / state.TrueAirspeed));
    }
  }
  bearing += Angle::degrees(small_rand());
}


void AircraftSim::update_state()  {

  const GlidePolar &glide_polar = task_manager.get_glide_polar();
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
  case FinalGlide:
    if (positive(stat.solution_remaining.VOpt)) {
      state.TrueAirspeed = stat.solution_remaining.VOpt*speed_factor;
    } else {
      state.TrueAirspeed = glide_polar.get_VbestLD();
    }
    state.Vario = -glide_polar.SinkRate(state.TrueAirspeed)*parms.sink_factor;
    update_bearing();
    break;
  case Climb:
    state.TrueAirspeed = glide_polar.get_Vmin();
    bearing += Angle::degrees(fixed_20+small_rand());
    state.Vario = climb_rate*parms.climb_factor;
    break;
  };
  state.NettoVario = state.Vario+glide_polar.SinkRate(state.TrueAirspeed);
}

#define fixed_300 fixed(300)
#define fixed_1500 fixed(1500)

fixed
AircraftSim::target_height()  
{
  if (task_manager.getActiveTaskPoint()) {
    return max(fixed_300, task_manager.getActiveTaskPoint()->get_elevation());
  } else {
    return fixed_300;
  }
}

void AircraftSim::update_mode()  
{
  
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
    if ((awp>0) && 
        (task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)) {
      print_mode("# mode fg\n");
      acstate = FinalGlide;
    } else {
      if (state.NavAltitude<=target_height()) {
        print_mode("# mode climb\n");
        acstate = Climb;
      }
    }
    break;
  case FinalGlide:
    if (task_manager.get_stats().total.solution_remaining.AltitudeDifference<-fixed_20) {
      print_mode("# mode climb\n");
      acstate = Climb;
    }
    break;
  case Climb:
    if ((awp>0) && 
        (task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)) {
      print_mode("# mode fg\n");
      acstate = FinalGlide;
    } else if (state.NavAltitude>=fixed_1500) {
      acstate = Cruise;
      print_mode("# mode cruise\n");
    }
    break;
  };
}

GeoPoint AircraftSim::endpoint(const Angle &bear) const
{
  GeoPoint ref = GeoVector(state.TrueAirspeed, bear).end_point(state.Location);
  return GeoVector(state.wind.norm, 
                   state.wind.bearing+ Angle::degrees(fixed_180)).end_point(ref);
}

void AircraftSim::integrate() {
  state.TrackBearing = bearing;
  state.Speed = endpoint(bearing).distance(state.Location);
  state.Location = endpoint(bearing);
  state.NavAltitude += state.Vario;
  state.AirspaceAltitude += state.Vario;
  state.Time += fixed_one;
}

bool AircraftSim::Update()  {

  update_state();
  
  integrate();
  
  update_mode();

  task_manager.update(state, state_last);
  task_manager.update_idle(state);
  task_manager.update_auto_mc(state, fixed_zero);
  
  state_last = state;

  state.flying_state_moving(state.Time);
  
  if (!far()) {
    wait_prompt(time());
    
    awp++;
    if (awp>= w.size()) {
      return false;
    } 
    task_manager.setActiveTaskPoint(awp);
  }

  if (goto_target) {
    if (task_manager.getActiveTaskPointIndex() < awp) {
      // manual advance
      task_manager.setActiveTaskPoint(awp);
      if (verbose>1) {
        printf("# manual advance to %d\n",awp);
      }
    }
  }
  if (task_manager.getActiveTaskPointIndex() > awp) {
    awp = task_manager.getActiveTaskPointIndex();
  }
  if (awp>= w.size()) {
    return false;
  } 
  if (short_flight && awp>=1) {
    return false;
  }
  if (task_manager.get_common_stats().task_finished) {
    return false;
  }

  return true;
}

void AircraftSim::print(std::ostream &f) {
  f << state.Time << " " 
    <<  state.Location.Longitude << " " 
    <<  state.Location.Latitude << " "
    <<  state.NavAltitude << "\n";
}

fixed AircraftSim::time() {
  return state.Time;
}


void 
AircraftSim::set_wind(const fixed speed, const Angle direction) 
{
  state.wind.norm = speed;
  state.wind.bearing = direction;
}
