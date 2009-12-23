
#include "harness_aircraft.hpp"
#include "test_debug.hpp"
#include "Math/Geometry.hpp"

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
  goto_target(_goto_target),
  speed_factor(1.0),
  climb_rate(2.0),
  short_flight(false)
{
  if (task_manager.task_size()<=1) {
    short_flight = true;
    // cheat for non-ordered tasks
    w.push_back(GEOPOINT(0.1,0.1));
    if (task_manager.task_size()>0) {
      w.push_back(task_manager.random_point_in_task(0, fixed(random_mag)));
    } else {
      w.push_back(GEOPOINT(1.0,0.0));
    }
  } else {
    for (unsigned i=0; i<task_manager.task_size(); i++) {
      w.push_back(task_manager.random_point_in_task(i, fixed(random_mag)));
    }
  }
  
  state.Location = w[0];
  state_last.Location = w[0];
  state.NavAltitude = 1500.0;
  state.Time = 0.0;
  state.WindSpeed = 0.0;
  state.WindDirection = 0;
  state.Speed = 16.0;
  
  bearing = 0;
  awp= 0;
  
  acstate = Cruise;
}

GEOPOINT AircraftSim::target(TaskManager &task_manager) {
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

bool AircraftSim::far(TaskManager &task_manager) {

  AbstractTaskFactory *fact = task_manager.get_factory();
  bool entered = fact->has_entered(awp);

  if (task_manager.task_size()==1) {
    // cheat for non-ordered tasks
    const ElementStat stat = task_manager.get_stats().current_leg;
    return (stat.remaining.get_distance()>100.0);
  } else if (task_manager.task_size()==0) {
    return w[1].distance(state.Location)>state.Speed;
  }
  if (goto_target && (awp>0)) {
    const ElementStat stat = task_manager.get_stats().current_leg;
    return (stat.remaining.get_distance()>100.0) || !entered;
  } else {
    fixed dc = w[awp].distance(state.Location);
    if (!positive(awp)) {
      return (dc>state.Speed);
    } else {
      return (dc>state.Speed) || !entered;
    }
  }
}

static const fixed fixed_1000 = 1000;
static const fixed fixed_20 = 20;

fixed AircraftSim::small_rand() {
  return heading_filt.update(bearing_noise*(2.0*rand()/RAND_MAX)-bearing_noise);
}

void AircraftSim::update_bearing(TaskManager& task_manager) {
  const ElementStat stat = task_manager.get_stats().current_leg;
  fixed bct = stat.solution_remaining.CruiseTrackBearing;

  if (goto_target && (awp>0)) {
    bearing = stat.solution_remaining.Vector.Bearing;

    if (enable_bestcruisetrack && (stat.solution_remaining.Vector.Distance>fixed_1000)) {
      bearing = bct;      
    }

  } else {
    bearing = state.Location.bearing(target(task_manager));
  }

  fixed b_best = bearing;
  fixed e_best = fixed_360;
  for (fixed bear= fixed_zero; bear<fixed_360; bear+= fixed_two) {
    fixed b_this = state.Location.bearing(endpoint(bear));
    fixed e_this = fabs(::AngleLimit180(b_this-bearing));
    if (e_this<e_best) {
      e_best = e_this;
      b_best = bear;
    }    
  }
  bearing = b_best+small_rand();
}


void AircraftSim::update_state(TaskManager &task_manager)  {

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
    state.Vario = -glide_polar.SinkRate(state.TrueAirspeed)*sink_factor;
    update_bearing(task_manager);
    break;
  case Climb:
    state.TrueAirspeed = glide_polar.get_Vmin();
    bearing += fixed_20+small_rand();
    state.Vario = climb_rate*climb_factor;
    break;
  };
  state.NettoVario = state.Vario+glide_polar.SinkRate(state.TrueAirspeed);
}

static const fixed fixed_300 = 300;
static const fixed fixed_1500 = 1500;

fixed
AircraftSim::target_height(TaskManager &task_manager)  
{
  if (task_manager.getActiveTaskPoint()) {
    return max(fixed_300, task_manager.getActiveTaskPoint()->get_elevation());
  } else {
    return fixed_300;
  }
}

void AircraftSim::update_mode(TaskManager &task_manager)  
{
  
  const ElementStat stat = task_manager.get_stats().current_leg;
  
  switch (acstate) {
  case Cruise:
    if ((awp>0) && 
        (task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)) {
      print_mode("# mode fg\n");
      acstate = FinalGlide;
    } else {
      if (state.NavAltitude<=target_height(task_manager)) {
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

GEOPOINT AircraftSim::endpoint(const fixed bear) const
{
  GEOPOINT ref;
  ref = GeoVector(state.TrueAirspeed,bear).end_point(state.Location);
  return GeoVector(state.WindSpeed,state.WindDirection+fixed_180).end_point(ref);
}

void AircraftSim::integrate() {
  state.TrackBearing = bearing;
  state.Speed = endpoint(bearing).distance(state.Location);
  state.Location = endpoint(bearing);
  state.NavAltitude += state.Vario;
  state.Time += fixed_one;
}

bool AircraftSim::advance(TaskManager &task_manager)  {

  update_state(task_manager);
  
  integrate();
  
  update_mode(task_manager);

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

#ifdef DO_PRINT
void AircraftSim::print(std::ostream &f4) {
  f4 << state.Time << " " 
     <<  state.Location.Longitude << " " 
     <<  state.Location.Latitude << " "
     <<  state.NavAltitude << "\n";
}
#endif

fixed AircraftSim::time() {
  return state.Time;
}


void 
AircraftSim::set_wind(const double speed, const double direction) 
{
  state.WindSpeed = speed;
  state.WindDirection = direction;
}
