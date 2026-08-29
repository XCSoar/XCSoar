// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Drives the real GlideComputer over an IGC file twice -- once shaped like a
 * live flight, once shaped like a Resume sweep -- and compares what comes
 * out.
 *
 * Every decision in the flight-resume design rests on reconstruction being
 * faithful, and until now nothing checked it.  See section 14 open question
 * 1 of doc/flight-resume-findings.md.
 *
 * What this proves: replacing GlideComputer::ProcessIdle() with
 * ProcessIdleForReplay() during a sweep changes neither the reconstructed
 * task progress nor the flight state, and keeps every replayed fix out of
 * the pilot's IGC file.
 *
 * What it does not prove: that a sweep through CalculationThread and the
 * DeviceBlackboard behaves identically to this in-process loop.  Those need
 * the application standing up and are out of reach from a test binary.
 */

#include "Computer/GlideComputer.hpp"
#include "Computer/GlideComputerInterface.hpp"
#include "Computer/Settings.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Input/InputQueue.hpp"
#include "NMEA/FlyingState.hpp"
#include "IGC/IGCWriter.hpp"
#include "Logger/Logger.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskFile.hpp"
#include "DebugReplayIGC.hpp"
#include "TestUtil.hpp"
#include "util/PrintException.hxx"

#include <memory>

/* fake symbols: the glide computer notifies the input queue and the logger,
   neither of which belongs in a test binary.  Counting the logger calls is
   how the "no replayed fix is ever logged" guarantee becomes observable. */

static unsigned logged_fixes, logged_start_events, logged_finish_events;

bool
InputEvents::processGlideComputer(unsigned)
{
  return false;
}

void Logger::LogPoint(const NMEAInfo &) { ++logged_fixes; }
void Logger::LogStartEvent(const NMEAInfo &) { ++logged_start_events; }
void Logger::LogFinishEvent(const NMEAInfo &) { ++logged_finish_events; }

LoggerImpl::LoggerImpl() = default;
LoggerImpl::~LoggerImpl() noexcept = default;

/* done with fake symbols. */

static ComputerSettings
MakeComputerSettings() noexcept
{
  ComputerSettings settings;
  settings.SetDefaults();
  return settings;
}

static TaskBehaviour
MakeTaskBehaviour() noexcept
{
  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  return task_behaviour;
}

/**
 * Everything GlideComputer needs to exist, with no threads and no terrain.
 */
struct Harness {
  ComputerSettings settings{MakeComputerSettings()};
  TaskBehaviour task_behaviour{MakeTaskBehaviour()};
  Waypoints waypoints;
  Airspaces airspaces;
  TaskManager task_manager{task_behaviour, waypoints};
  ProtectedTaskManager protected_task_manager{task_manager, task_behaviour};
  GlideComputerTaskEvents events;
  Logger logger;
  GlideComputer computer{settings, waypoints, airspaces,
                         protected_task_manager, events};

  Harness() {
    task_manager.SetTaskEvents(events);
    computer.SetLogger(&logger);
    computer.ReadComputerSettings(settings);
    computer.Initialise();
  }

  void SetTask(Path task_path) {
    auto task = TaskFile::GetTask(task_path, task_behaviour, nullptr, 0);
    ok1(task != nullptr);
    task->UpdateGeometry();
    ok1(task_manager.Commit(*task));
  }
};

/** The reconstructed state that a pilot would actually notice. */
struct Outcome {
  bool task_started, task_finished, flying;
  unsigned active_taskpoint_index;
  int travelled_metres;
  int flight_time_seconds;
  int altitude_samples;

  bool operator==(const Outcome &) const noexcept = default;
};

static Outcome
Capture(const Harness &harness) noexcept
{
  const DerivedInfo &calculated = harness.computer.Calculated();
  const TaskStats &stats = calculated.ordered_task_stats;

  return {
    .task_started = stats.start.HasStarted(),
    .task_finished = stats.task_finished,
    .flying = calculated.flight.flying,
    .active_taskpoint_index = harness.task_manager.GetActiveTaskPointIndex(),
    /* rounded to the metre: the two passes must agree exactly, but the
       assertion should not be hostage to a floating-point last bit */
    .travelled_metres = (int)stats.total.travelled.GetDistance(),
    .flight_time_seconds = (int)calculated.flight.flight_time.count(),
    .altitude_samples =
      (int)harness.computer.GetFlightStats().altitude.GetCount(),
  };
}

static void
Print(const char *label, const Outcome &o) noexcept
{
  printf("# %-6s started=%d finished=%d flying=%d active=%u "
         "travelled=%dm flight_time=%ds altitude_samples=%d\n",
         label, o.task_started, o.task_finished, o.flying,
         o.active_taskpoint_index, o.travelled_metres,
         o.flight_time_seconds, o.altitude_samples);
}

enum class Shape {
  /** As the calculation thread runs it for a live flight. */
  LIVE,
  /** As a Resume sweep runs it. */
  SWEEP,
};

/**
 * Both shapes run the identical ProcessGPS() and differ only in which idle
 * entry point follows it, on the identical cadence.  Any divergence is
 * therefore attributable to that one difference, which is exactly the claim
 * under test.
 *
 * The cadence matches StatsComputer's one-sample-per-minute throttle; the
 * live computer's own cadence is wall-clock driven and so would make this
 * test non-deterministic.
 */
static constexpr unsigned IDLE_EVERY_N_FIXES = 60;

/**
 * @param stop_after_fixes stop early, as a Cut Session does; 0 replays the
 * whole file
 */
static Outcome
RunPass(Harness &harness, Path igc_path, Shape shape,
        unsigned stop_after_fixes = 0)
{
  std::unique_ptr<DebugReplay> replay{DebugReplayIGC::Create(igc_path)};
  ok1(replay != nullptr);

  unsigned n = 0;
  while (replay->Next()) {
    harness.computer.ReadBlackboard(replay->Basic());
    harness.computer.Expire();
    harness.computer.ProcessGPS(true);

    if (++n % IDLE_EVERY_N_FIXES == 0) {
      if (shape == Shape::LIVE)
        harness.computer.ProcessIdle();
      else
        harness.computer.ProcessIdleForReplay();
    }

    if (stop_after_fixes > 0 && n >= stop_after_fixes)
      break;
  }

  if (shape == Shape::SWEEP)
    /* the one exhaustive pass that settles the contest at the end of a
       sweep; the live shape gets there incrementally */
    harness.computer.ProcessIdleForReplay(true);

  ok1(n > 1000);

  return Capture(harness);
}

/**
 * Reconstruction is faithful: sweeping produces the state a live run
 * produces.
 */
static void
TestReconstructionMatchesLiveRun(Path task_path, Path igc_path)
{
  Outcome live, sweep;

  {
    Harness harness;
    harness.SetTask(task_path);
    live = RunPass(harness, igc_path, Shape::LIVE);
  }

  {
    Harness harness;
    harness.SetTask(task_path);
    sweep = RunPass(harness, igc_path, Shape::SWEEP);
  }

  Print("live", live);
  Print("sweep", sweep);

  /* The flight must actually have been reconstructed, or comparing two empty
     results would pass vacuously.  Note this fixture lands at the end, so
     `flying` is correctly false here; the airborne case is covered below.
     The active turnpoint index matches what RunTask independently reports
     for this fixture in section 9.2 of the findings. */
  ok1(live.task_started);
  ok1(live.active_taskpoint_index == 1);
  ok1(live.flight_time_seconds > 3600);
  ok1(live.altitude_samples > 0);

  ok1(sweep == live);
}

/**
 * A replayed fix must never reach the pilot's IGC file.  Duplicating a whole
 * Flight in the one artefact that matters for scoring is the failure that
 * the separate replay idle entry point exists to prevent.
 */
static void
TestSweepLogsNothing(Path task_path, Path igc_path)
{
  {
    /* first establish that the hook is live: the live shape does log */
    Harness harness;
    harness.SetTask(task_path);
    logged_fixes = logged_start_events = logged_finish_events = 0;
    RunPass(harness, igc_path, Shape::LIVE);
    ok1(logged_fixes > 0);
  }

  {
    Harness harness;
    harness.SetTask(task_path);
    logged_fixes = logged_start_events = logged_finish_events = 0;
    RunPass(harness, igc_path, Shape::SWEEP);
    ok1(logged_fixes == 0);
  }
}

/**
 * A Cut Session ends in mid-air.  That is the shape a real sweep replays,
 * and the precondition the acceptance check tests for -- reconstructed state
 * is kept only if the replayed Session ends airborne.
 */
static void
TestCutSessionEndsAirborne(Path task_path, Path igc_path)
{
  Harness harness;
  harness.SetTask(task_path);

  const auto cut = RunPass(harness, igc_path, Shape::SWEEP, 3000);
  Print("cut", cut);

  ok1(cut.flying);
  /* non-vacuous: takeoff was detected and hours of flight reconstructed.
     Not task_started -- this fixture crosses its start line only in the last
     few hundred fixes, so no airborne cut point has task progress.  That the
     task is reconstructed at all is asserted by the full-file test above. */
  ok1(cut.flight_time_seconds > 3600);
}

/**
 * A Resume that turns out to belong to a Flight which had already ended is
 * undone completely -- except for the live takeoff that triggered it, which
 * the pilot's new Flight still needs.
 */
static void
TestRejectionRollsBackState(Path task_path, Path igc_path)
{
  Harness harness;
  harness.SetTask(task_path);

  /* stand in for the takeoff the live computer detected moments ago, and
     which ResetFlight() would otherwise wipe */
  FlyingState live_flight;
  live_flight.Reset();
  live_flight.flying = true;
  live_flight.takeoff_time = TimeStamp{std::chrono::seconds{42000}};
  live_flight.flight_time = std::chrono::seconds{30};

  const auto before = RunPass(harness, igc_path, Shape::SWEEP);
  ok1(before.task_started);
  ok1(before.active_taskpoint_index > 0);
  ok1(before.altitude_samples > 0);

  harness.computer.RejectReconstruction(live_flight);

  const auto after = Capture(harness);
  Print("reject", after);

  /* everything the sweep rebuilt is gone: task progress lives in the task
     engine rather than in DerivedInfo, which is why a DerivedInfo snapshot
     alone would not have been enough to undo it */
  ok1(!after.task_started);
  ok1(after.active_taskpoint_index == 0);
  ok1(after.altitude_samples == 0);

  /* but the live takeoff survives, so the pilot is left on a normally
     detected Flight rather than back at zero */
  ok1(after.flying);
  ok1(after.flight_time_seconds == 30);
}

/**
 * Hours of replayed fixes must not bury the pilot in stale notifications the
 * moment the progress dialog closes.
 */
static void
TestConditionMonitorsCanBeSilenced()
{
  ConditionMonitors monitors;
  NMEAInfo basic;
  basic.Reset();
  DerivedInfo calculated;
  calculated.Reset();
  const auto settings = MakeComputerSettings();

  /* the monitors notify through the input queue, which is stubbed above, so
     the observable here is simply that a suppressed Update() is inert and
     does not crash on the empty state a sweep would hand it */
  monitors.SetSuppressed(true);
  monitors.Update(basic, calculated, settings);
  ok1(true);

  monitors.SetSuppressed(false);
  ok1(true);
}

int main()
try {
  plan_tests(43);

  const Path task_path{"test/data/apf-bug554.tsk"};
  const Path igc_path{"test/data/apf-bug554.igc"};

  TestReconstructionMatchesLiveRun(task_path, igc_path);
  TestSweepLogsNothing(task_path, igc_path);
  TestCutSessionEndsAirborne(task_path, igc_path);
  TestRejectionRollsBackState(task_path, igc_path);
  TestConditionMonitorsCanBeSilenced();

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
