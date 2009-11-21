
#include "harness_aircraft.hpp"
#include "test_debug.hpp"

/*
  void scan_airspaces(Airspaces &airspaces, bool do_print) {
  }
*/

GEOPOINT AircraftSim::get_next() const {
  return w[awp+1];
}

AircraftSim::AircraftSim(int _test_num, const TaskManager& task_manager,
  bool _goto_target):
  test_num(_test_num),
  heading_filt(8.0),
  goto_target(_goto_target)
{
  for (unsigned i=0; i<task_manager.get_task_size(); i++) {
    w.push_back(task_manager.random_point_in_task(i));
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
    return w[awp+1];
  }
}

bool AircraftSim::far(TaskManager &task_manager) {
  if (goto_target && (awp>0)) {
    const ElementStat stat = task_manager.get_stats().current_leg;
    return stat.remaining.get_distance()>100.0;
  } else {
    return (w[awp+1].distance(state.Location)>state.Speed);
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
    bearing = state.Location.bearing(target(task_manager))+small_rand();
  }
}

void AircraftSim::update_state(TaskManager &task_manager,
                               GlidePolar &glide_polar)  {
  
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
    state.Speed = stat.solution_remaining.VOpt;
    sinkrate = glide_polar.SinkRate(state.Speed);        
    update_bearing(task_manager);
    if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)
        && (awp>1)) {
      printf("# mode fg\n");
      acstate = FinalGlide;
    } else {
      if (state.Altitude<=300) {
        printf("# mode climb\n");
        acstate = Climb;
      }
    }
    break;
  case FinalGlide:
    state.Speed = stat.solution_remaining.VOpt*0.97;
    sinkrate = glide_polar.SinkRate(state.Speed);
    update_bearing(task_manager);
    break;
  case Climb:
    state.Speed = 25.0;
    bearing += 20+small_rand();
    sinkrate = -glide_polar.get_mc();
    if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)
        && (awp>1)) {
      printf("# mode fg\n");
      acstate = FinalGlide;
    } else if (state.Altitude>=1500) {
      acstate = Cruise;
      printf("# mode cruise\n");
    }
    break;
  };
}

void AircraftSim::integrate() {
  state.Location = GeoVector(state.Speed,bearing).end_point(state.Location);
  state.Altitude -= sinkrate;
  state.Time += 1.0;
}

bool AircraftSim::advance(TaskManager &task_manager,
                          GlidePolar &glide_polar)  {
  
  update_state(task_manager, glide_polar);
  
  integrate();
  
  task_manager.update(state, state_last);
  task_manager.update_idle(state);
  
  state_last = state;
  
  if (!far(task_manager)) {
    
    if ((test_num==1) && (n_samples>500)) {
      return false;
    }
    wait_prompt(time());
    
    awp++;
    if (awp== w.size()) {
      return false;
    } else {
      if (goto_target) {
        if (task_manager.getActiveTaskPointIndex() != awp) {
          // manual advance
          task_manager.setActiveTaskPoint(awp);
        }
      }
    }
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

