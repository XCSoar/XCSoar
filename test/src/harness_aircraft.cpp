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

GeoPoint AircraftSim::get_next() const {
  return w[awp];
}

AircraftSim::AircraftSim(int _test_num, const TaskManager& task_manager,
                         double random_mag,
                         bool _goto_target):
  test_num(_test_num),
  heading_filt(fixed(8)),
  goto_target(_goto_target),
  speed_factor(1.0),
  climb_rate(2.0),
  short_flight(false)
{
  if (task_manager.task_size()<=1) {
    short_flight = true;
    // cheat for non-ordered tasks
    w.push_back(GeoPoint(Angle::degrees(fixed(0.1)), Angle::degrees(fixed(0.1))));
    if (task_manager.task_size()>0) {
      w.push_back(task_manager.random_point_in_task(0, fixed(random_mag)));
    } else {
      w.push_back(GeoPoint(Angle::degrees(fixed_one), Angle::degrees(fixed_zero)));
    }
  } else {
    for (unsigned i=0; i<task_manager.task_size(); i++) {
      w.push_back(task_manager.random_point_in_task(i, fixed(random_mag)));
    }
  }
  
  state.Location = w[0];
  state_last.Location = w[0];
  state.NavAltitude = fixed(start_alt);
  state.Time = fixed_zero;
  state.wind.norm = fixed_zero;
  state.wind.bearing = Angle();
  state.Speed = fixed(16);

  // start with aircraft moving since this isn't a real replay (no time on ground)
  for (unsigned i=0; i<10; i++) {
    state.flying_state_moving(state.Time);
  }
  
  bearing = Angle();
  awp= 0;
  
  acstate = Cruise;
}

GeoPoint AircraftSim::target(TaskManager &task_manager) {
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

static const fixed fixed_1000(1000);
static const fixed fixed_20(20);

fixed AircraftSim::small_rand() {
  return fixed(heading_filt.update(fixed(bearing_noise) *
                                   (fixed_two * rand() / RAND_MAX) -
                                   fixed(bearing_noise)));
}

void AircraftSim::update_bearing(TaskManager& task_manager) {
  const ElementStat stat = task_manager.get_stats().current_leg;
  Angle bct = stat.solution_remaining.CruiseTrackBearing;

  if (goto_target && (awp>0)) {
    bearing = stat.solution_remaining.Vector.Bearing;

    if (enable_bestcruisetrack && (stat.solution_remaining.Vector.Distance>fixed_1000)) {
      bearing = bct;      
    }

  } else {
    bearing = state.Location.bearing(target(task_manager));
  }

  Angle b_best = bearing;
  fixed e_best = fixed_360;
  const Angle delta = Angle::degrees(fixed_two);
  for (Angle bear= delta; bear<Angle::degrees(fixed_360); bear+= delta) {
    Angle b_this = state.Location.bearing(endpoint(bear));
    fixed e_this = (b_this-bearing).as_delta().magnitude_degrees();
    if (e_this<e_best) {
      e_best = e_this;
      b_best = bear;
    }    
  }
  bearing = b_best+Angle::degrees(small_rand());
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
    state.Vario = -glide_polar.SinkRate(state.TrueAirspeed)*fixed(sink_factor);
    update_bearing(task_manager);
    break;
  case Climb:
    state.TrueAirspeed = glide_polar.get_Vmin();
    bearing += Angle::degrees(fixed_20+small_rand());
    state.Vario = climb_rate*fixed(climb_factor);
    break;
  };
  state.NettoVario = state.Vario+glide_polar.SinkRate(state.TrueAirspeed);
}

static const fixed fixed_300(300);
static const fixed fixed_1500(1500);

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

GeoPoint AircraftSim::endpoint(const Angle &bear) const
{
  GeoPoint ref;
  ref = GeoVector(state.TrueAirspeed, bear).end_point(state.Location);
  return GeoVector(state.wind.norm, 
                   state.wind.bearing+ Angle::degrees(fixed_180)).end_point(ref);
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
  task_manager.update_auto_mc(state, fixed_zero);
  
  state_last = state;

  state.flying_state_moving(state.Time);
  
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
void AircraftSim::print(std::ostream &f) {
  f << state.Time << " " 
    <<  state.Location.Longitude << " " 
    <<  state.Location.Latitude << " "
    <<  state.NavAltitude << "\n";
}
#endif

fixed AircraftSim::time() {
  return state.Time;
}


void 
AircraftSim::set_wind(const fixed speed, const Angle direction) 
{
  state.wind.norm = speed;
  state.wind.bearing = direction;
}
