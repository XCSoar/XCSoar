#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "TaskEventsPrint.hpp"
#include "Replay/IgcReplay.hpp"
#include "Task/TaskManager.hpp"
#include "UtilsText.hpp"
#ifdef DO_PRINT
#include <fstream>
#endif

class ReplayLoggerSim: public IgcReplay
{
public:
  ReplayLoggerSim(): 
    IgcReplay(),
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
  virtual void on_reset() {}
  virtual void on_stop() {}
  virtual void on_bad_file() {}

  void on_advance(const GeoPoint &loc,
                  const fixed speed, const Angle bearing,
                  const fixed alt, const fixed baroalt, const fixed t) {

    state.Location = loc;
    state.Speed = speed;
    state.TrackBearing = bearing;
    state.NavAltitude = alt;
    state.Time = t;
    if (positive(t)) {
      started = true;
    }
  }
};

static bool
test_replay(const OLCRules olc_type)
{
#ifdef DO_PRINT
  std::ofstream f("results/res-sample.txt");
#endif

  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  AIRCRAFT_STATE state_last;

  TaskBehaviour task_behaviour;

  TaskEventsPrint default_events(verbose);
  TaskManager task_manager(default_events,
                           waypoints);

  task_manager.set_glide_polar(glide_polar);

  task_manager.get_task_behaviour().olc_rules = olc_type;
  task_manager.get_task_behaviour().enable_olc = true;

  ReplayLoggerSim sim;
  TCHAR szFilename[MAX_PATH];
  ConvertCToT(szFilename, replay_file.c_str());
  sim.SetFilename(szFilename);
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

      if (sim.state.Speed> glide_polar.get_Vtakeoff()) {
        sim.state.flying_state_moving(sim.state.Time);
      } else {
        sim.state.flying_state_stationary(sim.state.Time);
      }

      task_manager.update(sim.state, state_last);
      task_manager.update_idle(sim.state);
      task_manager.update_auto_mc(sim.state, fixed_zero);
  
      state_last = sim.state;

#ifdef DO_PRINT
      if (verbose) {
        sim.print(f);
        f.flush();
      }
      if (do_print) {
        task_manager.print(sim.state);
      }
#endif
      do_print = (++print_counter % output_skip ==0) && verbose;
    }
    time_last = sim.state.Time;
  };
  sim.Stop();

  const CommonStats& stats = task_manager.get_common_stats();
  printf("# OLC dist %g speed %g time %g\n",
         (double)stats.distance_olc,
         (double)(stats.speed_olc*fixed(3.6)),
         (double)stats.time_olc);

  if (verbose) {
    distance_counts();
  }

  return true;
}


int main(int argc, char** argv) 
{
  output_skip = 60;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(3);

  ok(test_replay(OLC_Classic),"replay classic",0);
  ok(test_replay(OLC_Sprint),"replay sprint",0);
  ok(test_replay(OLC_FAI),"replay fai",0);

  return exit_status();
}

