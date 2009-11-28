#include "harness_flight.hpp"
#include "harness_airspace.hpp"
#ifdef DO_PRINT
#include <fstream>
#endif

Airspaces *airspaces = NULL;

double time_elapsed=0.0;
double time_planned=1.0;
double time_remaining=0;
double calc_cruise_efficiency=1.0;

double aat_min_time(int test_num) {
  TaskBehaviour beh;
  switch (test_num) {
  case 2:
    return 3600*3.8;
  default:
    return beh.aat_min_time;
  }
}

int wind_to_mag(int n_wind) {
  if (n_wind) {
    return ((n_wind-1)/4+1)*5;
  }
  return 0;
}

int wind_to_dir(int n_wind) {
  if (n_wind) {
    return 90*((n_wind-1)%4);
  }
  return 0;
}

const char* wind_name(int n_wind) {
  static char buffer[80];
  sprintf(buffer,"%d m/s @ %d",wind_to_mag(n_wind),wind_to_dir(n_wind));
  return buffer;
}


bool run_flight(TaskManager &task_manager,
                GlidePolar &glide_polar,
                bool goto_target,
                double random_mag,
                int n_wind,
                const double speed_factor) 
{
  srand(0);

  AircraftSim ac(0, task_manager, random_mag, goto_target);
  unsigned print_counter=0;

  if (n_wind) {
    ac.set_wind(wind_to_mag(n_wind),wind_to_dir(n_wind));
  }
  ac.set_speed_factor(speed_factor);

#ifdef DO_PRINT
  std::ofstream f4("results/res-sample.txt");
#endif

  bool do_print = verbose;
  bool first = true;

  time_elapsed=0.0;
  time_planned=1.0;
  time_remaining=0;
  calc_cruise_efficiency=1.0;

  do {

    if ((task_manager.getActiveTaskPointIndex()==1) && first 
        && (task_manager.get_stats().total.TimeElapsed>10)) {
      time_remaining = task_manager.get_stats().total.TimeRemaining;
      first = false;

      time_planned = task_manager.get_stats().total.TimePlanned;
      if (verbose>1) {
        printf("# time remaining %g\n", time_remaining);
        printf("# time planned %g\n", time_planned);
      }

    }

    if (do_print) {
      task_manager.print(ac.get_state());
      ac.print(f4);
      f4.flush();
    }

    if (airspaces) {
      scan_airspaces(ac.get_state(), *airspaces, do_print, 
                     ac.target(task_manager));
    }

    n_samples++;

    do_print = (++print_counter % output_skip ==0) && verbose;

  } while (ac.advance(task_manager, glide_polar));

  if (verbose) {
    task_manager.print(ac.get_state());
    ac.print(f4);
    f4 << "\n";
    f4.flush();
    task_report(task_manager, "end of task\n");
  }
  wait_prompt(0);

  time_elapsed = task_manager.get_stats().total.TimeElapsed;
  time_planned = task_manager.get_stats().total.TimePlanned;
  calc_cruise_efficiency = task_manager.get_stats().cruise_efficiency;

  if (verbose) {
    distance_counts();
  }
  return true;
}


bool test_flight(int test_num, int n_wind, const double speed_factor,
                 const bool auto_mc) 
{
  // multipurpose flight test

  GlidePolar glide_polar(2.0,0.0,0.0);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskBehaviour task_behaviour;
  task_behaviour.auto_mc = auto_mc;
  task_behaviour.aat_min_time = aat_min_time(test_num);

  TaskEvents default_events;  default_events.verbose = verbose;

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  bool goto_target = false;

  switch (test_num) {
  case 0:
    goto_target = true;
    break;
  case 2:
    goto_target = true;
    break;
  case 7:
    goto_target = true;
    break;
  default:
    break;
  };

  test_task(task_manager, waypoints, test_num);

  return run_flight(task_manager, glide_polar, goto_target, target_noise, n_wind,
                    speed_factor);
}

bool test_flight_times(int test_num, int n_wind) 
{

  // tests whether elapsed/planned times are consistent
  // there will be small error due to start location

  bool fine = test_flight(test_num, n_wind);
  const double t_rat = fabs(time_elapsed/time_planned-1.0);
  fine &= t_rat<0.02;

  if ((verbose) || !fine) {
    printf("# time remaining %g\n", time_remaining);
    printf("# time elapsed %g\n", time_elapsed);
    printf("# time planned %g\n", time_planned);
    printf("# time ratio error (elapsed/planned) %g\n", t_rat);
  }
  return fine;
}


bool test_speed_factor(int test_num, int n_wind) 
{
  // flying at opt speed should be minimum time flight!

  double te0, te1, te2;

  test_flight(test_num, n_wind, 1.0);
  te0 = time_elapsed;

  test_flight(test_num, n_wind, 0.8);
  te1 = time_elapsed;
  // time of this should be higher than nominal
  ok(te0<te1, test_name("vopt slow or",test_num, n_wind), 0);

  test_flight(test_num, n_wind, 1.2);
  te2 = time_elapsed;
  // time of this should be higher than nominal
  ok(te0<te2, test_name("vopt fast or",test_num, n_wind), 0);

  bool retval = (te0<te1) && (te0<te2);
  if (verbose || !retval) {
    printf("# sf 0.8 time_elapsed_rat %g\n",te1/te0);
    printf("# sf 1.2 time_elapsed_rat %g\n",te2/te0);
  }
  return retval;
}


bool test_cruise_efficiency(int test_num, int n_wind) 
{

  // tests functionality of cruise efficiency calculations

  double ce0, ce1, ce2, ce3, ce4, ce5, ce6;

  bearing_noise = 0.0;
  target_noise = 0.1;

  test_flight(test_num, n_wind);
  ce0 = calc_cruise_efficiency;

  // wandering
  bearing_noise = 40.0;
  test_flight(test_num, n_wind);
  ce1 = calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  ok (ce0>ce1, test_name("ce wandering",test_num, n_wind),0);
  if (ce0<=ce1 || verbose) {
    printf("# calc cruise efficiency %g\n", calc_cruise_efficiency);
  }

  // flying too slow
  bearing_noise = 0.0;
  test_flight(test_num, n_wind, 0.8);
  ce2 = calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  ok (ce0>ce2, test_name("ce speed slow",test_num, n_wind),0);
  if (ce0<=ce2 || verbose) {
    printf("# calc cruise efficiency %g\n", calc_cruise_efficiency);
  }

  // flying too fast
  bearing_noise = 0.0;
  test_flight(test_num, n_wind, 1.2);
  ce3 = calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  ok (ce0>ce3, test_name("ce speed fast",test_num, n_wind),0);
  if (ce0<=ce3 || verbose) {
    printf("# calc cruise efficiency %g\n", calc_cruise_efficiency);
  }

  // higher than expected cruise sink
  sink_factor = 1.2;
  test_flight(test_num, n_wind);
  ce4 = calc_cruise_efficiency;
  ok (ce0>ce4, test_name("ce high sink",test_num, n_wind),0);
  // cruise efficiency of this should be lower than nominal
  sink_factor = 1.0;
  if (ce0<=ce4 || verbose) {
    printf("# calc cruise efficiency %g\n", calc_cruise_efficiency);
  }

  // slower than expected climb
  climb_factor = 0.8;
  test_flight(test_num, n_wind);
  ce5 = calc_cruise_efficiency;
  ok (ce0>ce5, test_name("ce slow climb",test_num, n_wind),0);
  // cruise efficiency of this should be lower than nominal
  climb_factor = 1.0;
  if (ce0<=ce5 || verbose) {
    printf("# calc cruise efficiency %g\n", calc_cruise_efficiency);
  }

  // lower than expected cruise sink; 
  sink_factor = 0.8;
  test_flight(test_num, n_wind);
  ce6 = calc_cruise_efficiency;
  ok (ce0<ce6, test_name("ce low sink",test_num, n_wind),0);
  // cruise efficiency of this should be greater than nominal
  sink_factor = 1.0;
  if (ce0>=ce6 || verbose) {
    printf("# calc cruise efficiency %g\n", calc_cruise_efficiency);
  }

  bool retval = (ce0>ce1) && (ce0>ce2) && (ce0>ce3) && (ce0>ce4) && (ce0>ce5)
    && (ce0<ce6);
  if (verbose || !retval) {
    printf("# ce nominal %g\n",ce0);
    printf("# ce wandering %g\n",ce1);
    printf("# ce speed slow %g\n",ce2);
    printf("# ce speed fast %g\n",ce3);
    printf("# ce high sink %g\n",ce4);
    printf("# ce slow climb %g\n",ce5);
    printf("# ce low sink %g\n",ce6);
  }
  return retval;
}


bool test_aat(int test_num, int n_wind) 
{

  // test whether flying to targets in an AAT task produces
  // elapsed (finish) times equal to desired time with 1.5% tolerance

  bool fine = test_flight(test_num, n_wind);
  double min_time = aat_min_time(test_num);

  const double t_ratio = fabs(time_elapsed/min_time-1.0);
  fine &= (t_ratio<0.015);
  if (!fine || verbose) {
    printf("# time ratio error (elapsed/target) %g\n", t_ratio);
  }
  return fine;
}


bool test_automc(int test_num, int n_wind) 
{

  // test whether flying by automc (starting above final glide)
  // arrives home faster than without

  test_flight(test_num, n_wind, 1.0, false);
  double t0 = time_elapsed;

  test_flight(test_num, n_wind, 1.0, true);
  double t1 = time_elapsed;

  bool fine = (t1/t0<1.015);
  ok(fine,test_name("faster with auto mc on",test_num, n_wind),0);
  
  if (!fine || verbose) {
    printf("# time ratio %g\n", t1/t0);
  }
  return fine;
}

bool test_bestcruisetrack(int test_num, int n_wind)
{

  // tests whether following the cruise track which compensates for wind drift
  // produces routes that are more on track than if the route is allowed to drift
  // downwind during climbs

  // this test allows for a small error margin

  enable_bestcruisetrack = false;
  test_flight(test_num, n_wind);
  double t0 = time_elapsed;

  enable_bestcruisetrack = true;
  test_flight(test_num, n_wind);
  double t1 = time_elapsed;
  enable_bestcruisetrack = false;

  bool fine = (t1/t0<1.01);
  ok(fine,test_name("faster flying with bestcruisetrack",test_num, n_wind),0);
  
  if (!fine || verbose) {
    printf("# time ratio %g\n", t1/t0);
  }
  return fine;
}


bool test_abort(int n_wind)
{
  GlidePolar glide_polar(2.0,0.0,0.0);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskBehaviour task_behaviour;
//  task_behaviour.auto_mc = auto_mc;

  task_behaviour.all_off();

  TaskEvents default_events;  default_events.verbose = verbose;

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  test_task(task_manager, waypoints, 2);

  task_manager.abort();
  task_report(task_manager, "abort");

  return run_flight(task_manager, glide_polar, true, target_noise, n_wind);

}

bool test_goto(int n_wind, unsigned id, bool auto_mc)
{
  GlidePolar glide_polar(2.0,0.0,0.0);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskBehaviour task_behaviour;

  task_behaviour.all_off();
  task_behaviour.auto_mc = auto_mc;

  TaskEvents default_events;  default_events.verbose = verbose;

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  test_task(task_manager, waypoints, 2);

  task_manager.do_goto(*waypoints.lookup_id(id));
  task_report(task_manager, "goto");

  return run_flight(task_manager, glide_polar, true, target_noise, n_wind);
}


bool test_null()
{
  GlidePolar glide_polar(2.0,0.0,0.0);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskBehaviour task_behaviour;
//  task_behaviour.auto_mc = auto_mc;

  task_behaviour.all_off();

  TaskEvents default_events;  default_events.verbose = verbose;

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);
  task_report(task_manager, "null");

  return run_flight(task_manager, glide_polar, true, target_noise, 0);
}


bool test_airspace(const unsigned n_airspaces)
{
  airspaces = new Airspaces;
  setup_airspaces(*airspaces, n_airspaces);
  bool fine = test_flight(4,0);
  delete airspaces; airspaces = NULL;
  return fine;
}
