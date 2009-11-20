
#include "test_aircraft.hpp"
#include "test_debug.hpp"

/*
  void scan_airspaces(Airspaces &airspaces, bool do_print) {
  }
*/

GEOPOINT AircraftSim::get_next() const {
  return w[awp+1];
}

AircraftSim::AircraftSim(int _test_num, const TaskManager& task_manager):
  test_num(_test_num),
  heading_filt(8.0)
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

bool AircraftSim::far() {
  return (w[awp+1].distance(state.Location)>state.Speed);
}

double AircraftSim::small_rand() {
  return heading_filt.update(-40.0+rand()*80.0/RAND_MAX);
}

void AircraftSim::update_state(TaskManager &task_manager,
                               GlidePolar &glide_polar)  {
  
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
    state.Speed = stat.solution_remaining.VOpt;
    sinkrate = glide_polar.SinkRate(state.Speed);        
    bearing = state.Location.bearing(w[awp+1])+small_rand();
    if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)
        && (awp>1)) {
      printf("fg\n");
      acstate = FinalGlide;
    } else {
      if (state.Altitude<=300) {
        printf("climb\n");
        acstate = Climb;
      }
    }
    break;
  case FinalGlide:
    state.Speed = stat.solution_remaining.VOpt*0.97;
    sinkrate = glide_polar.SinkRate(state.Speed);
    bearing = state.Location.bearing(w[awp+1])+small_rand();
    break;
  case Climb:
    state.Speed = 25.0;
    bearing += 20+small_rand();
    sinkrate = -glide_polar.get_mc();
    if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)
        && (awp>1)) {
      printf("fg\n");
      acstate = FinalGlide;
    } else if (state.Altitude>=1500) {
      acstate = Cruise;
      printf("cruise\n");
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
  
  if (!far()) {
    
    if ((test_num==1) && (n_samples>500)) {
      return false;
    }
    wait_prompt(time());
    
    awp++;
    if (awp+1== w.size()) {
      return false;
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

