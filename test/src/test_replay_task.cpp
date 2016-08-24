#include "harness_aircraft.hpp"
#include "TaskEventsPrint.hpp"
#include "Replay/IgcReplay.hpp"
#include "Task/TaskManager.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Computer/FlyingComputer.hpp"
#include "NMEA/FlyingState.hpp"
#include "OS/ConvertPathName.hpp"
#include "OS/FileUtil.hpp"
#include "IO/FileLineReader.hpp"
#include "Task/LoadFile.hpp"
#include "NMEA/Info.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Util/PrintException.hxx"
#include "test_debug.hpp"

#include <fstream>

static OrderedTask *
task_load(const TaskBehaviour &task_behaviour)
{
  auto *task = LoadTask(task_file, task_behaviour);
  if (task != nullptr) {
    task->UpdateStatsGeometry();
    if (!task->CheckTask()) {
      delete task;
      return nullptr;
    }
  }

  return task;
}

class ReplayLoggerSim: public IgcReplay
{
public:
  explicit ReplayLoggerSim(std::unique_ptr<NLineReader> &&_reader)
    :IgcReplay(std::move(_reader)),
     started(false) {}

  AircraftState state;

  void print(std::ostream &f) {
    f << (double)state.time << " " 
      <<  (double)state.location.longitude.Degrees() << " "
      <<  (double)state.location.latitude.Degrees() << " "
      <<  (double)state.altitude << "\n";
  }
  bool started;

protected:
  virtual void OnReset() {}
  virtual void OnStop() {}

  void OnAdvance(const GeoPoint &loc,
                 const double speed, const Angle bearing,
                 const double alt, const double baroalt, const double t) {

    state.location = loc;
    state.ground_speed = speed;
    state.track = bearing;
    state.altitude = alt;
    state.time = t;
    if (t > 0) {
      started = true;
    }
  }
};

static bool
test_replay()
{
  Directory::Create(Path(_T("output/results")));
  std::ofstream f("output/results/res-sample.txt");

  GlidePolar glide_polar(4.0);
  Waypoints waypoints;
  AircraftState state_last;

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  task_behaviour.auto_mc = true;

  TaskManager task_manager(task_behaviour, waypoints);

  TaskEventsPrint default_events(verbose);
  task_manager.SetTaskEvents(default_events);

  glide_polar.SetBallast(1.0);

  task_manager.SetGlidePolar(glide_polar);

  OrderedTask* t = task_load(task_behaviour);
  if (t) {
    task_manager.Commit(*t);
    delete t;
    task_manager.Resume();
  } else {
    return false;
  }

  // task_manager.get_task_advance().get_advance_state() = TaskAdvance::AUTO;

  ReplayLoggerSim sim(std::make_unique<FileLineReaderA>(replay_file));
  sim.state.netto_vario = 0;

  bool do_print = verbose;
  unsigned print_counter=0;

  NMEAInfo basic;
  basic.Reset();

  while (sim.Update(basic) && !sim.started) {
  }
  state_last = sim.state;

  sim.state.wind.norm = 7;
  sim.state.wind.bearing = Angle::Degrees(330);

  auto time_last = sim.state.time;

//  uncomment this to manually go to first tp
//  task_manager.incrementActiveTaskPoint(1);

  FlyingComputer flying_computer;
  flying_computer.Reset();

  FlyingState flying_state;
  flying_state.Reset();

  while (sim.Update(basic)) {
    if (sim.state.time>time_last) {

      n_samples++;

      flying_computer.Compute(glide_polar.GetVTakeoff(),
                              sim.state, sim.state.time - time_last,
                              flying_state);
      sim.state.flying = flying_state.flying;

      task_manager.Update(sim.state, state_last);
      task_manager.UpdateIdle(sim.state);
      task_manager.UpdateAutoMC(sim.state, 0);
      task_manager.SetTaskAdvance().SetArmed(true);

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
    time_last = sim.state.time;
  };

  if (verbose) {
    PrintDistanceCounts();
    printf("# task elapsed %d (s)\n", (int)task_manager.GetStats().total.time_elapsed);
    printf("# task speed %3.1f (kph)\n", (int)task_manager.GetStats().total.travelled.GetSpeed()*3.6);
    printf("# travelled distance %4.1f (km)\n", 
           (double)task_manager.GetStats().total.travelled.GetDistance()/1000.0);
    printf("# scored distance %4.1f (km)\n", 
           (double)task_manager.GetStats().distance_scored/1000.0);
    if (task_manager.GetStats().total.time_elapsed > 0) {
      printf("# scored speed %3.1f (kph)\n", 
             (double)task_manager.GetStats().distance_scored/(double)task_manager.GetStats().total.time_elapsed*3.6);
    }
  }
  return true;
}


int main(int argc, char** argv) 
try {
  output_skip = 60;

  replay_file = Path(_T("test/data/apf-bug554.igc"));
  task_file = Path(_T("test/data/apf-bug554.tsk"));

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  plan_tests(1);

  ok(test_replay(),"replay task",0);

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
