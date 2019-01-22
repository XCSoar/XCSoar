#include "harness_aircraft.hpp"
#include "Replay/IgcReplay.hpp"
#include "Computer/TraceComputer.hpp"
#include "Computer/FlyingComputer.hpp"
#include "Engine/Contest/ContestManager.hpp"
#include "Computer/Settings.hpp"
#include "OS/ConvertPathName.hpp"
#include "OS/FileUtil.hpp"
#include "IO/FileLineReader.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "test_debug.hpp"
#include "Util/PrintException.hxx"

#include <fstream>

extern "C" {
#include "tap.h"
}

ContestResult official_score_classic,
  official_score_sprint,
  official_score_fai,
  official_score_plus;
double official_index;

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
  if (official.score <= 0) {
    return true;
  }
  auto e = fabs((official.score-estimated.score)/official.score);
  std::cout << "# Error (score) " << e << "\n";
  if (official.score > 0) {
    return (e<0.01);
  } else {
    // nothing to compare with
    return true; 
  }
}

inline void load_score_file(std::ifstream& fscore,
                            ContestResult& score)
{
  double tmp;
  fscore >> tmp; score.score = tmp;
  fscore >> tmp; score.distance = tmp;
  fscore >> tmp; double speed(tmp);
  if (speed>0) {
    score.time = 3600*score.distance/speed;
  } else {
    score.time = 0;
  }
  score.distance *= 1000;
}


inline void load_scores(unsigned &contest_handicap) {
  // replay_file
  const auto score_file = replay_file.WithExtension(_T(".txt"));
  if (verbose) {
    std::cout << "# replay file: " << replay_file << "\n";
    std::cout << "# score file: " << score_file << "\n";
  }
  const NarrowPathName score_file2(score_file);
  std::ifstream fscore(score_file2);
  double tmp;
  fscore >> tmp; official_index = tmp;
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
  explicit ReplayLoggerSim(std::unique_ptr<NLineReader> &&_reader)
    :IgcReplay(std::move(_reader)) {}

  void print(std::ostream &f, const MoreData &basic) {
    f << (double)basic.time << " " 
      <<  (double)basic.location.longitude.Degrees() << " " 
      <<  (double)basic.location.latitude.Degrees() << " "
      <<  (double)basic.nav_altitude << "\n";
  }
protected:
  virtual void OnReset() {}
};

static bool
test_replay(const Contest olc_type,
            const ContestResult &official_score)
{
  Directory::Create(Path(_T("output/results")));
  std::ofstream f("output/results/res-sample.txt");

  GlidePolar glide_polar(2);

  ReplayLoggerSim sim(std::make_unique<FileLineReaderA>(replay_file));

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

  FlyingComputer flying_computer;
  flying_computer.Reset();

  FlyingState flying_state;
  flying_state.Reset();

  TraceComputer trace_computer;

  ContestManager contest_manager(olc_type,
                                 trace_computer.GetFull(),
                                 trace_computer.GetFull(),
                                 trace_computer.GetSprint());
  contest_manager.SetHandicap(settings_computer.contest.handicap);

  DerivedInfo calculated;

  while (sim.Update(basic)) {
    n_samples++;

    flying_computer.Compute(glide_polar.GetVTakeoff(),
			    basic, calculated,
			    flying_state);

    calculated.flight.flying = true;
    
    trace_computer.Update(settings_computer, basic, calculated);
    
    contest_manager.UpdateIdle();
  
    if (verbose>1) {
      sim.print(f, basic);
      f.flush();
    }
    if (do_print) {
      PrintHelper::trace_print(trace_computer.GetFull(), basic.location);
    }
    do_print = (++print_counter % output_skip ==0) && verbose;
  };

  contest_manager.SolveExhaustive();

  if (verbose) {
    PrintDistanceCounts();
  }
  return compare_scores(official_score, 
                        contest_manager.GetStats().GetResult(0));
}


int main(int argc, char** argv) 
try {
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
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
