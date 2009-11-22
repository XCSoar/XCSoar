
#include "harness_aircraft.hpp"
#include "test_debug.hpp"
#include "Math/Geometry.hpp"

/*
  void scan_airspaces(Airspaces &airspaces, bool do_print) {
  }
*/

void print_mode(const char* mode) {
  if (verbose>1) {
    printf("%s",mode);
  }
}

GEOPOINT AircraftSim::get_next() const {
  return w[awp];
}

AircraftSim::AircraftSim(int _test_num, const TaskManager& task_manager,
                         double random_mag,
                         bool _goto_target):
  test_num(_test_num),
  heading_filt(8.0),
  goto_target(_goto_target)
{
  for (unsigned i=0; i<task_manager.get_task_size(); i++) {
    if (i==0) {
      w.push_back(task_manager.random_point_in_task(i, 1.0));
    } else {
      w.push_back(task_manager.random_point_in_task(i, random_mag));
    }
  }
  
  state.Location = w[0];
  state_last.Location = w[0];
  state.Altitude = 1500.0;
  state.Time = 0.0;
  state.WindSpeed = 0.0;
  state.WindDirection = 0;
  state.Speed = 16.0;
  
  bearing = 0;
  sinkrate = 0;
  awp= 0;
  
  acstate = Cruise;
}

GEOPOINT AircraftSim::target(TaskManager &task_manager) {
  if (goto_target && (awp>0)) {
    return task_manager.getActiveTaskPoint()->getLocation();
  } else {
    if (awp>0) {
      return w[awp];
    } else {
      return w[awp+1];
    }
  }
}

bool AircraftSim::far(TaskManager &task_manager) {
  if (goto_target && (awp>0)) {
    const ElementStat stat = task_manager.get_stats().current_leg;
    return stat.remaining.get_distance()>100.0;
  } else {
    if (awp>0) {
      double d0s = w[awp-1].distance(state.Location);
      double d01 = w[awp-1].distance(w[awp]); 
      if (d0s> d01) {
//        return false;
      }
    }
    double dc = w[awp].distance(state.Location);
    return (dc>state.Speed);
  }
}

double AircraftSim::small_rand() {
  return heading_filt.update(-40.0+rand()*80.0/RAND_MAX);
}

void AircraftSim::update_bearing(TaskManager& task_manager) {
  if (goto_target && (awp>0)) {
    const ElementStat stat = task_manager.get_stats().current_leg;
    bearing = stat.solution_remaining.Vector.Bearing;
  } else {
    bearing = state.Location.bearing(target(task_manager));
  }

  double b_best = bearing;
  double e_best = 370;
  for (double bear=0; bear<360; bear+= 2.0) {
    double b_this = state.Location.bearing(endpoint(bear));
    double e_this = fabs(::AngleLimit180(b_this-bearing));
    if (e_this<e_best) {
      e_best = e_this;
      b_best = bear;
    }    
  }
  bearing = b_best+small_rand();
}

void AircraftSim::update_state(TaskManager &task_manager,
                               GlidePolar &glide_polar)  {
  
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
    state.Speed = stat.solution_remaining.VOpt;
    sinkrate = glide_polar.SinkRate(state.Speed);        
    update_bearing(task_manager);
    break;
  case FinalGlide:
    if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)) {
      state.Speed = stat.solution_remaining.VOpt;
    } else {
      state.Speed = stat.solution_remaining.VOpt*0.9;
    }
    sinkrate = glide_polar.SinkRate(state.Speed);
    update_bearing(task_manager);
    break;
  case Climb:
    state.Speed = 25.0;
    bearing += 20+small_rand();
    sinkrate = -glide_polar.get_mc();
    break;
  };
}

void AircraftSim::update_mode(TaskManager &task_manager,
                               GlidePolar &glide_polar)  {
  
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
    if ((awp>0) && 
        (task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)) {
      print_mode("# mode fg\n");
      acstate = FinalGlide;
    } else {
      if (state.Altitude<=300) {
        print_mode("# mode climb\n");
        acstate = Climb;
      }
    }
    break;
  case FinalGlide:

    break;
  case Climb:
    if ((awp>0) && 
        (task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)) {
      print_mode("# mode fg\n");
      acstate = FinalGlide;
    } else if (state.Altitude>=1500) {
      acstate = Cruise;
      print_mode("# mode cruise\n");
    }
    break;
  };
}

GEOPOINT AircraftSim::endpoint(const double bear) const
{
  GEOPOINT ref;
  ref = GeoVector(state.Speed,bear).end_point(state.Location);
  return GeoVector(state.WindSpeed,state.WindDirection).end_point(ref);
}

void AircraftSim::integrate() {
  state.Location = endpoint(bearing);
  state.Altitude -= sinkrate;
  state.Time += 1.0;
}

bool AircraftSim::advance(TaskManager &task_manager,
                          GlidePolar &glide_polar)  {

  update_state(task_manager, glide_polar);
  
  integrate();
  
  update_mode(task_manager, glide_polar);

  task_manager.update(state, state_last);
  task_manager.update_idle(state);
  
  state_last = state;
  
  if (!far(task_manager)) {
    wait_prompt(time());
    
    awp++;
    if (awp>= w.size()) {
      return false;
    } 
  }

  if (goto_target) {
    if (task_manager.getActiveTaskPointIndex() < awp) {
      // manual advance
      task_manager.setActiveTaskPoint(awp);
    }
  }
  if (task_manager.getActiveTaskPointIndex() > awp) {
    awp = task_manager.getActiveTaskPointIndex();
  }
  if (awp>= w.size()) {
    return false;
  } 
  return true;
}

#ifdef DO_PRINT
void AircraftSim::print(std::ostream &f4) {
  f4 << state.Time << " " 
     <<  state.Location.Longitude << " " 
     <<  state.Location.Latitude << " "
     <<  state.Altitude << "\n";
}
#endif

double AircraftSim::time() {
  return state.Time;
}


void 
AircraftSim::set_wind(const double speed, const double direction) 
{
  state.WindSpeed = speed;
  state.WindDirection = direction;
}
