#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "TaskEventsPrint.hpp"
#include "Replay/IgcReplay.hpp"
#include "Task/TaskManager.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Computer/FlyingComputer.hpp"
#include "NMEA/FlyingState.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "IO/FileLineReader.hpp"
#include "Task/Deserialiser.hpp"
#include "XML/DataNodeXML.hpp"
#include "NMEA/Info.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <fstream>

static OrderedTask* task_load(OrderedTask* task) {
  PathName szFilename(task_file.c_str());
  DataNode *root = DataNodeXML::Load(szFilename);
  if (!root)
    return NULL;

  Deserialiser des(*root);
  des.Deserialise(*task);
  if (task->CheckTask()) {
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
  ReplayLoggerSim(NLineReader *reader)
    :IgcReplay(reader),
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
                  const fixed speed, const Angle bearing,
                  const fixed alt, const fixed baroalt, const fixed t) {

    state.location = loc;
    state.ground_speed = speed;
    state.track = bearing;
    state.altitude = alt;
    state.time = t;
    if (positive(t)) {
      started = true;
    }
  }
};

static bool
test_replay()
{
  Directory::Create(_T("output/results"));
  std::ofstream f("output/results/res-sample.txt");

  GlidePolar glide_polar(fixed(4.0));
  Waypoints waypoints;
  AircraftState state_last;

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  task_behaviour.auto_mc = true;
  task_behaviour.enable_trace = false;

  TaskManager task_manager(task_behaviour, waypoints);

  TaskEventsPrint default_events(verbose);
  task_manager.SetTaskEvents(default_events);

  glide_polar.SetBallast(fixed(1.0));

  task_manager.SetGlidePolar(glide_polar);

  OrderedTask* blank = new OrderedTask(task_manager.GetTaskBehaviour());

  OrderedTask* t = task_load(blank);
  if (t) {
    task_manager.Commit(*t);
    task_manager.Resume();
  } else {
    return false;
  }

  // task_manager.get_task_advance().get_advance_state() = TaskAdvance::AUTO;

  FileLineReaderA *reader = new FileLineReaderA(replay_file.c_str());
  if (reader->error()) {
    delete reader;
    return false;
  }

  ReplayLoggerSim sim(reader);
  sim.state.netto_vario = fixed(0);

  bool do_print = verbose;
  unsigned print_counter=0;

  NMEAInfo basic;
  basic.Reset();

  while (sim.Update(basic) && !sim.started) {
  }
  state_last = sim.state;

  sim.state.wind.norm = fixed(7);
  sim.state.wind.bearing = Angle::Degrees(330);

  fixed time_last = sim.state.time;

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
      task_manager.UpdateAutoMC(sim.state, fixed(0));
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
    if (positive(task_manager.GetStats().total.time_elapsed)) {
      printf("# scored speed %3.1f (kph)\n", 
             (double)task_manager.GetStats().distance_scored/(double)task_manager.GetStats().total.time_elapsed*3.6);
    }
  }
  return true;
}


int main(int argc, char** argv) 
{
  output_skip = 60;

  replay_file = "test/data/apf-bug554.igc";
  task_file = "test/data/apf-bug554.tsk";

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  plan_tests(1);

  ok(test_replay(),"replay task",0);

  return exit_status();
}

