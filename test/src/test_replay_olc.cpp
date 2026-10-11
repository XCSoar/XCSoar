// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_aircraft.hpp"
#include "Replay/IgcReplay.hpp"
#include "Computer/TraceComputer.hpp"
#include "Computer/FlyingComputer.hpp"
#include "Engine/Contest/ContestManager.hpp"
#include "Computer/Settings.hpp"
#include "Geo/SearchPointVector.hpp"
#include "system/ConvertPathName.hpp"
#include "system/FileUtil.hpp"
#include "io/FileLineReader.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "test_debug.hpp"
#include "util/PrintException.hxx"

#include <algorithm>
#include <cmath>
#include <fstream>

extern "C" {
#include "tap.h"
}

using namespace std::chrono;

ContestResult official_score_classic,
  official_score_sprint,
  official_score_fai,
  official_score_plus,
  official_score_league;
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
    score.time = FloatDuration{hours{1}} * score.distance / speed;
  } else {
    score.time = {};
  }
  score.distance *= 1000;
}


inline void load_scores(unsigned &contest_handicap) {
  // replay_file
  const auto score_file = replay_file.WithSuffix(".txt");
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
  load_score_file(fscore, official_score_league);
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
    f << basic.time.ToDuration().count() << " "
      <<  (double)basic.location.longitude.Degrees() << " " 
      <<  (double)basic.location.latitude.Degrees() << " "
      <<  (double)basic.nav_altitude << "\n";
  }
protected:
  virtual void OnReset() {}
};

/**
 * OLC Plus stores the plus score in result 2. Result 0 is classic.
 * The sprint trace only keeps the latest two hours, so that contest
 * has to be solved while the flight is replayed.
 */
static ContestStatistics
ReplayFlight(TraceComputer &trace_computer, unsigned handicap)
{
  Directory::Create(Path("output/results"));
  std::ofstream f("output/results/res-sample.txt");

  GlidePolar glide_polar(2);

  ReplayLoggerSim sim(std::make_unique<FileLineReaderA>(replay_file));

  ComputerSettings settings_computer;
  settings_computer.SetDefaults();
  settings_computer.contest.enable = true;
  settings_computer.contest.handicap = handicap;

  bool do_print = verbose;
  unsigned print_counter = 0;

  MoreData basic;
  basic.Reset();

  FlyingComputer flying_computer;
  flying_computer.Reset();

  FlyingState flying_state;
  flying_state.Reset();

  ContestManager sprint(Contest::OLC_SPRINT,
                        trace_computer.GetFull(),
                        trace_computer.GetFull(),
                        trace_computer.GetSprint());
  sprint.SetHandicap(handicap);

  DerivedInfo calculated;

  while (sim.Update(basic)) {
    n_samples++;

    flying_computer.Compute(glide_polar.GetVTakeoff(),
                            basic, calculated,
                            flying_state);

    calculated.flight.flying = true;

    trace_computer.Update(settings_computer, basic, calculated);
    sprint.UpdateIdle();

    if (verbose > 1) {
      sim.print(f, basic);
      f.flush();
    }
    if (do_print)
      PrintHelper::trace_print(trace_computer.GetFull(), basic.location);
    do_print = (++print_counter % output_skip == 0) && verbose;
  }

  sprint.SolveExhaustive();

  if (verbose)
    PrintDistanceCounts();

  return sprint.GetStats();
}

static ContestStatistics
Solve(Contest type, const TraceComputer &trace_computer,
      unsigned handicap)
{
  ContestManager contest_manager(type,
                                 trace_computer.GetFull(),
                                 trace_computer.GetFull(),
                                 trace_computer.GetSprint());
  contest_manager.SetHandicap(handicap);
  contest_manager.SolveExhaustive();
  return contest_manager.GetStats();
}

static bool
SameScore(double got, double expected) noexcept
{
  return fabs(got - expected) < 0.02;
}

/** Relative tolerance used for the frozen scores of this IGC. */
static bool
ScoreNear(double got, double expected) noexcept
{
  return expected > 0 && fabs(got - expected) / expected < 0.01;
}

static bool
SameDistance(double got, double expected) noexcept
{
  return fabs(got - expected) < 1.;
}

static double
Handicapped(double raw, unsigned handicap) noexcept
{
  return 100. * raw / handicap;
}

/**
 * DMSt distance score: km * (1 + bonus) * 100 / index, or 0 below
 * 50 points.
 */
static double
DMStScore(double distance_m, double bonus, unsigned handicap) noexcept
{
  const double score =
    Handicapped(distance_m / 1000. * (1. + bonus), handicap);
  return score >= 50. ? score : 0.;
}

static double
PathLength(const ContestTraceVector &path) noexcept
{
  double total = 0;
  for (unsigned i = 1; i < path.size(); ++i)
    total += path[i - 1].DistanceTo(path[i].GetLocation());
  return total;
}

/**
 * Triangle legs are points 1-2, 2-3 and 3-1. Point 0 and point 4 are
 * the closing pair.
 */
static double
TriangleLength(const ContestTraceVector &path) noexcept
{
  return path[1].DistanceTo(path[2].GetLocation())
    + path[2].DistanceTo(path[3].GetLocation())
    + path[3].DistanceTo(path[1].GetLocation());
}

/** FAI/OLC: 28% shortest below 500 km, else 25% and longest under 45%. */
static bool
TriangleLegsOk(const ContestTraceVector &path) noexcept
{
  if (path.size() != 5)
    return false;

  const double a = path[1].DistanceTo(path[2].GetLocation());
  const double b = path[2].DistanceTo(path[3].GetLocation());
  const double c = path[3].DistanceTo(path[1].GetLocation());
  const double total = a + b + c;
  if (!(total > 0))
    return false;

  const double shortest = std::min({a, b, c});
  const double longest = std::max({a, b, c});
  if (total < 500000.)
    return shortest * 25 >= total * 7;

  return shortest * 4 >= total && longest * 20 < total * 9;
}

static bool
test_olc(Contest type, const ContestResult &official,
         const TraceComputer &trace, unsigned handicap)
{
  const auto stats = Solve(type, trace, handicap);
  /* OLC Plus stores the plus score in result 2. Result 0 is classic. */
  const unsigned which = type == Contest::OLC_PLUS ? 2 : 0;
  return compare_scores(official, stats.GetResult(which));
}

static bool
test_dmst(const ContestStatistics &stats, unsigned handicap)
{
  const auto &quad = stats.GetResult(0);
  const auto &triangle = stats.GetResult(1);
  const auto &out_return = stats.GetResult(2);
  const auto &best = stats.GetResult(3);

  /*
   * 0asljd01.igc at handicap 120. Quad (no bonus) beats the triangle
   * and the out-and-return, so result 3 is the quad score.
   */
  if (!ScoreNear(quad.score, 322.813) ||
      !ScoreNear(triangle.score, 276.918) ||
      !ScoreNear(out_return.score, 308.289) ||
      !ScoreNear(best.score, 322.813))
    return false;

  if (!SameScore(quad.score, DMStScore(quad.distance, 0., handicap)) ||
      !SameScore(triangle.score,
                 DMStScore(triangle.distance, 0.4, handicap)) ||
      !SameScore(out_return.score,
                 DMStScore(out_return.distance, 0.3, handicap)))
    return false;

  /* Same priority as DMStFree::GetBestType(): triangle, then OR. */
  const ContestResult *winner = &quad;
  if (triangle.score >= quad.score &&
      triangle.score >= out_return.score)
    winner = &triangle;
  else if (out_return.score >= quad.score)
    winner = &out_return;

  if (!SameScore(best.score, winner->score) ||
      !SameDistance(best.distance, winner->distance))
    return false;

  const auto &tri_path = stats.GetSolution(1);
  if (tri_path.size() != 5 || !TriangleLegsOk(tri_path))
    return false;
  if (tri_path[0].DistanceTo(tri_path[4].GetLocation()) > 1000.)
    return false;
  if (!SameDistance(triangle.distance, TriangleLength(tri_path)))
    return false;

  /* Two legs: start, turnpoint, finish. */
  const auto &or_path = stats.GetSolution(2);
  if (or_path.size() != 3 ||
      !SameDistance(out_return.distance, PathLength(or_path)))
    return false;

  /* Four legs. */
  const auto &quad_path = stats.GetSolution(0);
  if (quad_path.size() != 5 ||
      !SameDistance(quad.distance, PathLength(quad_path)))
    return false;

  return true;
}

static bool
test_xcontest_triangle(const ContestStatistics &stats, unsigned handicap)
{
  /* Result 1 is the triangle. Result 0 is the free flight. */
  const auto &result = stats.GetResult(1);
  const auto &path = stats.GetSolution(1);

  /* 0asljd01.igc at handicap 120, after the closing gap is removed. */
  if (!ScoreNear(result.score, 275.084))
    return false;

  if (path.size() != 5 || !TriangleLegsOk(path))
    return false;
  if (path[0].DistanceTo(path[4].GetLocation()) > 1000.)
    return false;

  /*
   * The scored distance has the trace's start-to-finish gap removed,
   * and that gap must stay within 20% of the triangle.
   */
  const double length = TriangleLength(path);
  const double removed = length - result.distance;
  if (!(removed >= 0 && removed <= 0.2 * length))
    return false;
  if (fabs(result.distance - 235787.) > 1.)
    return false;

  /* is_fai is fixed true in XContestTriangle, so 1.4 points per km. */
  const double expected = Handicapped(result.distance * 0.0014, handicap);
  return SameScore(result.score, expected);
}

static bool
test_charron(const ContestStatistics &stats, unsigned handicap)
{
  const auto &result = stats.GetResult(0);
  const auto &path = stats.GetSolution(0);

  /* 0asljd01.igc at handicap 120. Longer than 200 km, so six legs. */
  if (!ScoreNear(result.score, 1364.11))
    return false;

  if (path.size() != 7 || !(result.distance >= 200000.))
    return false;
  if (!SameDistance(result.distance, PathLength(path)))
    return false;

  for (unsigned i = 1; i < path.size(); ++i)
    if (path[i - 1].DistanceTo(path[i].GetLocation()) < 20000.)
      return false;

  const double expected = Handicapped(result.distance * 4. / 1000.,
                                      handicap);
  return SameScore(result.score, expected);
}

/**
 * SIS-AT score from the documented hull formula:
 * V = hull perimeter minus the closing leg, S = path distance,
 * points = (V + S) / 2 km, then the handicap.
 */
static double
SisAtScore(const ContestTraceVector &path, double distance,
           unsigned handicap)
{
  SearchPointVector spv;
  spv.reserve(path.size());
  for (const auto &point : path)
    spv.emplace_back(point.GetLocation());

  spv.PruneInterior();

  double hull = 0;
  if (spv.size() > 1) {
    for (unsigned i = 0; i + 1 < spv.size(); ++i)
      hull += spv[i].DistanceTo(spv[i + 1].GetLocation());
    hull += spv.back().DistanceTo(spv.front().GetLocation());
  }

  const double closing =
    path.front().DistanceTo(path.back().GetLocation());
  const double zigzag_free = hull - closing;
  return Handicapped((zigzag_free + distance) / 2000., handicap);
}

static bool
test_sis_at(const ContestStatistics &stats, unsigned handicap)
{
  const auto &result = stats.GetResult(0);
  const auto &path = stats.GetSolution(0);

  /* 0asljd01.igc at handicap 120. Six legs, hull score below classic. */
  if (!ScoreNear(result.score, 316.664))
    return false;

  if (path.size() != 7)
    return false;
  if (!SameDistance(result.distance, PathLength(path)))
    return false;

  return SameScore(result.score,
                   SisAtScore(path, result.distance, handicap));
}

int main(int argc, char** argv)
try {
  if (!ParseArgs(argc, argv))
    return 0;

  plan_tests(9);

  unsigned handicap = 0;
  load_scores(handicap);

  TraceComputer trace_computer;
  const auto sprint = ReplayFlight(trace_computer, handicap);

  ok(test_olc(Contest::OLC_LEAGUE, official_score_league,
              trace_computer, handicap),
     "replay league", 0);
  ok(test_olc(Contest::OLC_FAI, official_score_fai,
              trace_computer, handicap),
     "replay fai", 0);
  ok(test_olc(Contest::OLC_CLASSIC, official_score_classic,
              trace_computer, handicap),
     "replay classic", 0);
  ok(compare_scores(official_score_sprint, sprint.GetResult(0)),
     "replay sprint", 0);
  ok(test_olc(Contest::OLC_PLUS, official_score_plus,
              trace_computer, handicap),
     "replay plus", 0);

  const auto dmst = Solve(Contest::DMST, trace_computer, handicap);
  ok(test_dmst(dmst, handicap), "replay dmst", 0);

  const auto xcontest = Solve(Contest::XCONTEST, trace_computer, handicap);
  ok(test_xcontest_triangle(xcontest, handicap),
     "replay xcontest triangle", 0);

  const auto charron = Solve(Contest::CHARRON, trace_computer, handicap);
  ok(test_charron(charron, handicap), "replay charron", 0);

  const auto sis_at = Solve(Contest::SIS_AT, trace_computer, handicap);
  ok(test_sis_at(sis_at, handicap), "replay sis-at", 0);

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
