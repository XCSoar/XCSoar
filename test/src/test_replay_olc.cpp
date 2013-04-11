#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "Replay/IgcReplay.hpp"
#include "Computer/TraceComputer.hpp"
#include "Computer/FlyingComputer.hpp"
#include "Engine/Contest/ContestManager.hpp"
#include "Computer/Settings.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "IO/FileLineReader.hpp"
#include "Navigation/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

#include <fstream>

ContestResult official_score_classic,
  official_score_sprint,
  official_score_fai,
  official_score_plus;
fixed official_index;

inline void output_score(const char* header,
                         const ContestResult& score)
{
  std::cout << header << "\n";
  PrintHelper::print(score);
}

inline bool compare_scores(const ContestResult& official,
                           const ContestResult& estimated)
{
  if (verbose) {
    output_score("#  Official:", official);
    output_score("#  Estimated:", estimated);
  }
  if (!positive(official.score)) {
    return true;
  }
  fixed e = fabs((official.score-estimated.score)/official.score);
  std::cout << "# Error (score) " << e << "\n";
  if (positive(official.score)) {
    return (e<fixed(0.01));
  } else {
    // nothing to compare with
    return true; 
  }
}

inline void load_score_file(std::ifstream& fscore,
                            ContestResult& score)
{
  double tmp;
  fscore >> tmp; score.score = (fixed)tmp;
  fscore >> tmp; score.distance = (fixed)tmp;
  fscore >> tmp; fixed speed(tmp);
  if (speed>fixed(0)) {
    score.time = fixed(3600)*score.distance/speed;
  } else {
    score.time = fixed(0);
  }
  score.distance *= fixed(1000);
}


inline void load_scores(unsigned &contest_handicap) {
  // replay_file
  int index = replay_file.find_last_of(".");
  std::string score_file = replay_file.substr(0, index) + ".txt";
  if (verbose) {
    std::cout << "# replay file: " << replay_file << "\n";
    std::cout << "# score file: " << score_file << "\n";
  }
  std::ifstream fscore(score_file.c_str());
  double tmp;
  fscore >> tmp; official_index = (fixed)tmp;
  load_score_file(fscore, official_score_classic);
  load_score_file(fscore, official_score_sprint);
  load_score_file(fscore, official_score_fai);
  load_score_file(fscore, official_score_plus);
  fscore.close();

  contest_handicap = (unsigned)official_index;

  if (contest_handicap==0) { // in case file load failed
    contest_handicap = 100;
  }
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

  void OnAdvance(const GeoPoint &loc,
                  const fixed speed, const Angle bearing,
                  const fixed alt, const fixed baroalt, const fixed t) {

    state.location = loc;
    state.ground_speed = speed;
    state.track = bearing;
    state.altitude = alt;
    state.netto_vario = fixed(0);
    state.vario = fixed(0);
    state.time = t;
    if (positive(t)) {
      started = true;
    }
  }
};

static bool
test_replay(const Contest olc_type,
            const ContestResult &official_score)
{
  Directory::Create(_T("output/results"));
  std::ofstream f("output/results/res-sample.txt");

  GlidePolar glide_polar(fixed(2));
  AircraftState state_last;

  FileLineReaderA *reader = new FileLineReaderA(replay_file.c_str());
  if (reader->error()) {
    delete reader;
    return false;
  }

  ReplayLoggerSim sim(reader);

  ComputerSettings settings_computer;
  settings_computer.SetDefaults();
  settings_computer.contest.enable = true;
  load_scores(settings_computer.contest.handicap);

  if (verbose) {
    switch (olc_type) {
    case Contest::OLC_LEAGUE:
      std::cout << "# OLC-League\n";
      break;
    case Contest::OLC_SPRINT:
      std::cout << "# OLC-Sprint\n";
      break;
    case Contest::OLC_FAI:
      std::cout << "# OLC-FAI\n";
      break;
    case Contest::OLC_CLASSIC:
      std::cout << "# OLC-Classic\n";
      break;
    case Contest::OLC_PLUS:
      std::cout << "# OLC-Plus\n";
      break;
    default:
      std::cout << "# Unknown!\n";
      break;
    }
  }

  bool do_print = verbose;
  unsigned print_counter=0;

  MoreData basic;
  basic.Reset();

  while (sim.Update(basic) && !sim.started) {
  }
  state_last = sim.state;

  fixed time_last = sim.state.time;

  FlyingComputer flying_computer;
  flying_computer.Reset();

  FlyingState flying_state;
  flying_state.Reset();

  TraceComputer trace_computer;

  ContestManager contest_manager(olc_type,
                                 trace_computer.GetFull(),
                                 trace_computer.GetSprint());
  contest_manager.SetHandicap(settings_computer.contest.handicap);

  DerivedInfo calculated;

  while (sim.Update(basic)) {
    if (sim.state.time>time_last) {

      n_samples++;

      flying_computer.Compute(glide_polar.GetVTakeoff(),
                              sim.state, sim.state.time - time_last,
                              flying_state);

      calculated.flight.flying = flying_state.flying;

      trace_computer.Update(settings_computer, basic, calculated);

      contest_manager.UpdateIdle();
  
      state_last = sim.state;

      if (verbose>1) {
        sim.print(f);
        f.flush();
      }
      if (do_print) {
        PrintHelper::trace_print(trace_computer.GetFull(), sim.state.location);
      }
      do_print = (++print_counter % output_skip ==0) && verbose;
    }
    time_last = sim.state.time;
  };

  contest_manager.SolveExhaustive();

  if (verbose) {
    PrintDistanceCounts();
  }
  return compare_scores(official_score, 
                        contest_manager.GetStats().GetResult(0));
}


int main(int argc, char** argv) 
{
  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  plan_tests(5);

  ok(test_replay(Contest::OLC_LEAGUE, official_score_sprint),
     "replay league", 0);
  ok(test_replay(Contest::OLC_FAI, official_score_fai),
     "replay fai", 0);
  ok(test_replay(Contest::OLC_CLASSIC, official_score_classic),
     "replay classic", 0);
  ok(test_replay(Contest::OLC_SPRINT, official_score_sprint),
     "replay sprint", 0);
  ok(test_replay(Contest::OLC_PLUS, official_score_plus),
     "replay plus", 0);

  return exit_status();
}

