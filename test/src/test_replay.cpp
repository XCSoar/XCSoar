#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "TaskEventsPrint.hpp"
#include "ReplayLogger.hpp"
#include "Task/TaskManager.hpp"
#ifdef DO_PRINT
#include <fstream>
#endif

class ReplayLoggerSim: public ReplayLogger
{
public:
  ReplayLoggerSim(): 
    ReplayLogger(),
    started(false) {}

  AIRCRAFT_STATE state;

#ifdef DO_PRINT
  void print(std::ostream &f) {
    f << state.Time << " " 
      <<  state.Location.Longitude << " " 
      <<  state.Location.Latitude << " "
      <<  state.NavAltitude << "\n";
  }
#endif
  bool started;

protected:
  void on_advance(const GEOPOINT &loc,
                  const double speed, const double bearing,
                  const double alt, const double baroalt, const double t) {

    state.Location = loc;
    state.Speed = speed;
    state.TrackBearing = bearing;
    state.NavAltitude = alt;
    state.Time = t;
    if (t>0) {
      started = true;
    }
  }
};


bool 
test_replay(const OLCRules olc_type)
{
#ifdef DO_PRINT
  std::ofstream f("results/res-sample.txt");
#endif

  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  AIRCRAFT_STATE state_last;

  TaskBehaviour task_behaviour;

  task_behaviour.olc_rules = olc_type;
  task_behaviour.enable_olc = true;

  TaskEventsPrint default_events(verbose);
  TaskManager task_manager(default_events,
                           task_behaviour,
                           waypoints);

  task_manager.set_glide_polar(glide_polar);

  ReplayLoggerSim sim;
  sim.SetFilename(replay_file.c_str());
  sim.Start();

  bool do_print = verbose;
  unsigned print_counter=0;

  while (sim.Update() && !sim.started) {
  }
  state_last = sim.state;

  fixed time_last = sim.state.Time;

  while (sim.Update()) {
    if (sim.state.Time>time_last) {

      n_samples++;

      task_manager.update(sim.state, state_last);
      task_manager.update_idle(sim.state);
  
      state_last = sim.state;

#ifdef DO_PRINT
      if (do_print) {
        task_manager.print(sim.state);
        sim.print(f);
        f.flush();
      }
#endif
      do_print = (++print_counter % output_skip ==0) && verbose;
    }
    time_last = sim.state.Time;
  };
  sim.Stop();

  if (verbose) {
    distance_counts();
  }

  return true;
}


int main(int argc, char** argv) 
{
  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(3);

  ok(test_replay(OLC_Classic),"replay classic",0);
  ok(test_replay(OLC_Sprint),"replay sprint",0);
  ok(test_replay(OLC_FAI),"replay fai",0);

  return exit_status();
}

