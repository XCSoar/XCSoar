#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "TaskEventsPrint.hpp"
#include "Replay/IgcReplay.hpp"
#include "Task/TaskManager.hpp"
#include "UtilsText.hpp"
#include <fstream>

// #include "Wind/WindAnalyser.hpp"

#include "Util/Deserialiser.hpp"
#include "Util/DataNodeXML.hpp"

/*

    windanalyser.slot_newSample(Basic(), SetCalculated());
    windanalyser.slot_Altitude(Basic(), SetCalculated());


*/

static OrderedTask* task_load(OrderedTask* task) {
  TCHAR szFilename[MAX_PATH];
  ConvertCToT(szFilename, task_file.c_str());
  DataNode* root = DataNodeXML::load(szFilename);
  if (!root)
    return NULL;

  Deserialiser des(*root);
  des.deserialise(*task);
  if (task->check_task()) {
    delete root;
    return task;
  }
  delete task;
  delete root;
  return NULL;
}

class ReplayLoggerSim: public IgcReplay
{
public:
  ReplayLoggerSim(): 
    IgcReplay(),
    started(false) {}

  AIRCRAFT_STATE state;

  void print(std::ostream &f) {
    f << (double)state.Time << " " 
      <<  (double)state.Location.Longitude.value_degrees() << " " 
      <<  (double)state.Location.Latitude.value_degrees() << " "
      <<  (double)state.NavAltitude << "\n";
  }
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
    state.track = bearing;
    state.NavAltitude = alt;
    state.Time = t;
    if (positive(t)) {
      started = true;
    }
  }
};

static bool
test_replay()
{
  std::ofstream f("results/res-sample.txt");

  GlidePolar glide_polar(fixed(4.0));
  Waypoints waypoints;
  AIRCRAFT_STATE state_last;

  TaskBehaviour task_behaviour;

  TaskEventsPrint default_events(verbose);
  TaskManager task_manager(default_events,
                           waypoints);

  glide_polar.set_ballast(fixed(1.0));

  task_manager.set_glide_polar(glide_polar);
  task_manager.get_task_behaviour().auto_mc = true;
  task_manager.get_task_behaviour().enable_trace = false;

  OrderedTask* blank = 
    new OrderedTask(default_events, task_behaviour, glide_polar);

  OrderedTask* t = task_load(blank);
  if (t) {
    task_manager.commit(*t);
    task_manager.resume();
  } else {
    return false;
  }

  // task_manager.get_task_advance().get_advance_state() = TaskAdvance::AUTO;

  ReplayLoggerSim sim;
  sim.state.NettoVario = fixed_zero;

  TCHAR szFilename[MAX_PATH];
  ConvertCToT(szFilename, replay_file.c_str());
  sim.SetFilename(szFilename);

  sim.Start();

  bool do_print = verbose;
  unsigned print_counter=0;

  while (sim.Update() && !sim.started) {
  }
  state_last = sim.state;

  sim.state.wind.norm = fixed(7);
  sim.state.wind.bearing = Angle::degrees(fixed(330));

  fixed time_last = sim.state.Time;

//  uncomment this to manually go to first tp
//  task_manager.incrementActiveTaskPoint(1);

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
      task_manager.get_task_advance().set_armed(true);

      state_last = sim.state;

      if (verbose>1) {
        sim.print(f);
        f.flush();
      }
      if (do_print) {
        PrintHelper::taskmanager_print(task_manager, sim.state);
      }
      do_print = (++print_counter % output_skip ==0) && verbose;
    }
    time_last = sim.state.Time;
  };
  sim.Stop();

  if (verbose) {
    distance_counts();
    printf("# task elapsed %d (s)\n", (int)task_manager.get_stats().total.TimeElapsed);
    printf("# task speed %3.1f (kph)\n", (int)task_manager.get_stats().total.travelled.get_speed()*3.6);
    printf("# travelled distance %4.1f (km)\n", 
           (double)task_manager.get_stats().total.travelled.get_distance()/1000.0);
    printf("# scored distance %4.1f (km)\n", 
           (double)task_manager.get_stats().distance_scored/1000.0);
    if (task_manager.get_stats().total.TimeElapsed) {
      printf("# scored speed %3.1f (kph)\n", 
             (double)task_manager.get_stats().distance_scored/(double)task_manager.get_stats().total.TimeElapsed*3.6);
    }
  }
  return true;
}


int main(int argc, char** argv) 
{
  output_skip = 60;

  replay_file = "test/data/apf-bug554.igc";
  task_file = "test/data/apf-bug554.tsk";

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(1);

  ok(test_replay(),"replay task",0);

  return exit_status();
}

