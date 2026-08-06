// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Rebuilding a Flight that a crash or a power cut interrupted.
 *
 * Bound at GCE_TAKEOFF *before* AutoLogger start, so the whole judgement --
 * is there a Cut Session, does replaying it produce a Flight that was still
 * airborne -- is made before the logger commits to anything.  Only then is
 * the logger told to continue that file.
 *
 * The order matters because appending cannot be undone: once two Flights
 * share one IGC file, nothing can separate them again.  Reconstructed
 * computer state, by contrast, is cheap to discard.  So the irreversible
 * step goes last.
 *
 * Nothing is lost by sweeping first.  The calculation thread is suspended
 * either way, so no live fix is recorded during the sweep whether the logger
 * is open or not.
 */

#include "InputEvents.hpp"
#include "BackendComponents.hpp"
#include "CalculationThread.hpp"
#include "Components.hpp"
#include "Computer/GlideComputer.hpp"
#include "DataComponents.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Interface.hpp"
#include "Job/Job.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "IGC/CutSession.hpp"
#include "LocalPath.hpp"
#include "Logger/Logger.hpp"
#include "Repository/FileType.hpp"
#include "MainWindow.hpp"
#include "MergeThread.hpp"
#include "Message.hpp"
#include "NMEA/FlyingState.hpp"
#include "Operation/DeadlineOperationEnvironment.hpp"
#include "Protection.hpp"
#include "Replay/Replay.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "UIGlobals.hpp"

#include <chrono>

/**
 * How long the glide computer may stay paused rebuilding a Flight.
 *
 * Chosen against a measured worst case with little headroom: a real 6 h 18 m
 * Flight logged at ~1 Hz produced 20 513 fixes, which projects to 41-103 s on
 * a Kobo-class device.  A shorter budget would abort exactly when there is
 * most state to restore.  If measurement on real hardware lands near the top
 * of that range, the answer is a coarser sweep -- sampling fixes rather than
 * replaying all of them -- not a longer freeze.
 */
static constexpr auto SWEEP_TIME_BUDGET = std::chrono::seconds{120};

namespace {

/**
 * Detaches the terrain from the glide computer for as long as it exists.
 *
 * Without this the route planner and reach fan fire on nearly every replayed
 * cruise fix -- their 5 s throttle runs on GPS time, and cruise logging is
 * also 5 s -- which is by far the most expensive thing in the compute path.
 * The only thing lost is the contest trace's wind-drift factor, because task
 * height constraints read waypoint elevations rather than the raster.
 *
 * Terrain is loaded during startup and attached there, so by takeoff it is
 * live on any device with a map file: this has to be done deliberately, it is
 * not a consequence of timing.  The map keeps its own reference, and its draw
 * threads are suspended anyway.
 */
class ScopeDetachTerrain {
  GlideComputer &computer;
  RasterTerrain *const terrain;

public:
  explicit ScopeDetachTerrain(GlideComputer &_computer,
                              RasterTerrain *_terrain) noexcept
    :computer(_computer), terrain(_terrain) {
    computer.SetTerrain(nullptr);
  }

  ~ScopeDetachTerrain() noexcept {
    computer.SetTerrain(terrain);
  }
};

/**
 * Silences the six condition monitors for as long as it exists.
 *
 * A sweep replays hours of fixes through the same code a live flight uses,
 * so without this the pilot is buried in stale wind, sunset, final-glide,
 * terrain, AAT and landable-reachable notifications the moment the progress
 * dialog closes.
 */
class ScopeSuppressConditionMonitors {
  GlideComputer &computer;

public:
  explicit ScopeSuppressConditionMonitors(GlideComputer &_computer) noexcept
    :computer(_computer) {
    computer.SetConditionMonitorsSuppressed(true);
  }

  ~ScopeSuppressConditionMonitors() noexcept {
    computer.SetConditionMonitorsSuppressed(false);
  }
};

/**
 * SuspendAllThreads() deliberately does not cover MergeThread, on the grounds
 * that it touches no unprotected shared data.  The sweep drives it directly,
 * though, so a live MergeThread loop would interleave with it.
 *
 * Same pattern the Lua replay API uses.
 */
class ScopeSuspendMergeThread {
  MergeThread &thread;

public:
  explicit ScopeSuspendMergeThread(MergeThread &_thread) noexcept
    :thread(_thread) {
    thread.Suspend();
  }

  ~ScopeSuspendMergeThread() noexcept {
    thread.Resume();
  }
};

/**
 * Drives the replay, and stops it again before anything else unwinds --
 * which is the whole point of it.
 *
 * While a replay is active, DeviceBlackboard::Merge() gives replay_data
 * precedence over live GPS.  A calculation thread running in that state calls
 * ProcessIdle(), and so LogComputer::Run(), against a *replayed* fix: hours
 * of old track written into the pilot's IGC.  The replayed fixes also drive
 * task transitions and a backwards time warp that resets the flying state,
 * which then reads as "landed" and throws away the whole reconstruction.
 *
 * Construct this innermost, so it is destroyed first and live GPS is restored
 * while the threads are still suspended.  See
 */
class ScopeReplay {
  Replay &replay;

public:
  ScopeReplay(Replay &_replay, Path path)
    :replay(_replay) {
    replay.StartSweep(path);
  }

  ~ScopeReplay() noexcept {
    replay.Stop();
  }
};

/**
 * Stops anything reaching the pilot's IGC file, or the pilot, for as long as
 * it exists.
 *
 * The replay-safe idle entry point keeps LogComputer::Run() out of a sweep,
 * but that is not the only route from ProcessGPS to the IGC file.  A
 * replayed task start reaches GlideComputer::OnStartTask(), which
 * calls LogComputer::StartTask(), which writes an E record -- and
 * IGCWriter::LogEvent() appends a B record after it, as the IGC spec
 * requires.  OnFinishTask() is pure state capture; OnStartTask() is not.
 *
 * The damage is worse than a few stray records.  They land at the end of the
 * very file the sweep is reading, timestamped hours before the fixes around
 * them; the sweep replays them, jumps backwards in time, and FlyingComputer
 * resets the flying state.  A Flight that ended airborne then reads as landed
 * and the entire reconstruction is rolled back.
 *
 * Suppressing at the LogComputer closes every route at once.  It has to be a
 * flag rather than SetLogger(nullptr): that setter asserts the logger has not
 * already been set, and swapping it trips the assertion.
 */
class ScopeSuppressLogging {
  GlideComputer &computer;

public:
  explicit ScopeSuppressLogging(GlideComputer &_computer) noexcept
    :computer(_computer) {
    computer.SetLoggingSuppressed(true);
    computer.SetTaskEventsSuppressed(true);
  }

  ~ScopeSuppressLogging() noexcept {
    computer.SetLoggingSuppressed(false);
    computer.SetTaskEventsSuppressed(false);
  }
};

class ResumeFlightJob final : public Job {
  Replay &replay;
  MergeThread &merge_thread;
  CalculationThread &calc_thread;
  const unsigned total_fixes;

public:
  unsigned fixes_processed = 0;

  ResumeFlightJob(Replay &_replay, MergeThread &_merge_thread,
                  CalculationThread &_calc_thread,
                  unsigned _total_fixes) noexcept
    :replay(_replay), merge_thread(_merge_thread),
     calc_thread(_calc_thread), total_fixes(_total_fixes) {}

  /* virtual methods from class Job */
  void Run(OperationEnvironment &env) override {
    /* the backstop is expressed as cancellation, so a sweep that runs out of
       time keeps its partial result exactly as a cancelled one does */
    DeadlineOperationEnvironment deadline_env{env, SWEEP_TIME_BUDGET};

    fixes_processed = replay.SweepAllFixes(merge_thread, calc_thread,
                                           deadline_env, total_fixes);
  }
};

} // anonymous namespace

void
InputEvents::eventResumeFlight([[maybe_unused]] const char *misc)
{
  auto *logger = backend_components->igc_logger.get();
  if (logger == nullptr)
    return;

  if (logger->IsLoggerActive())
    /* the logger is already recording, so this is not the takeoff that
       follows a restart */
    return;

  const auto &basic = CommonInterface::Basic();

  if (basic.location_available && !basic.gps.real)
    /* simulator or replay, not a real Flight */
    return;

  /* The scan happens here rather than in the logger because its result must
     be validated before anything acts on it.  Nothing the logger owns is
     open yet, so the scan cannot pick up the file this Session is about to
     create. */
  const auto logs_path = LocalPath(GetFileTypeDefaultDir(FileType::IGC));
  const auto candidate = FindCutSession(logs_path, basic.date_time_utc);
  if (!candidate) {
    LogFormat("No Cut Session found");
    return;
  }

  auto *replay = backend_components->replay.get();
  auto *merge_thread = backend_components->merge_thread.get();
  auto *calc_thread = backend_components->calculation_thread.get();
  auto *glide_computer = backend_components->glide_computer.get();

  if (replay == nullptr || merge_thread == nullptr ||
      calc_thread == nullptr || glide_computer == nullptr)
    return;

  if (replay->IsActive())
    /* the pilot is replaying a file; this is not a real Flight */
    return;

  LogFormat("Cut Session found: %s (%u fixes)",
            candidate->path.c_str(), candidate->b_record_count);

  /* captured before the sweep overwrites it, so a rejected Resume can put
     the live takeoff back */
  const FlyingState live_flight = glide_computer->Calculated().flight;

  unsigned fixes_processed = 0;
  bool ends_airborne = false;
  bool failed = false;


  try {
    {
      /* the threads go first: everything below mutates state the
         calculation thread reads, so flipping it while that thread runs
         would be a data race, and restoring it before the thread resumes is
         equally necessary */
      ScopeSuspendMergeThread suspend_merge{*merge_thread};
      ScopeSuspendAllThreads suspend;

      ScopeDetachTerrain detach_terrain{*glide_computer,
                                        data_components->terrain.get()};
      ScopeSuppressConditionMonitors silence{*glide_computer};
      ScopeSuppressLogging no_logging{*glide_computer};

      /* innermost, so the replay is stopped and live GPS restored while the
         threads are still suspended */
      ScopeReplay sweep{*replay, candidate->path};

      ResumeFlightJob job{*replay, *merge_thread, *calc_thread,
                          candidate->b_record_count};

      /* the UI thread sits in a nested modal loop drawing the bar and
         handling Cancel while the worker sweeps; MainWindow::SuspendThreads()
         suspends only the map's threads, not dialog rendering */
      JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                _("Rebuilding flight"), job, true);

      fixes_processed = job.fixes_processed;


      /* one exhaustive pass to settle the contest over the whole rebuilt
         trace, still through the replay-safe entry point so that no replayed
         fix can reach the pilot's IGC file */
      if (fixes_processed > 0)
        glide_computer->ProcessIdleForReplay(true);

      /* read the verdict here, while the threads are still suspended: once
         they resume, live fixes move the flying state on and the answer is no
         longer about the replayed Session */
      ends_airborne = glide_computer->Calculated().flight.flying;

    }

  } catch (...) {
    LogError(std::current_exception(), "Flight resume failed");
    Message::AddMessage(_("Could not rebuild flight"));
    failed = true;
  }

  if (failed) {
    /* Whatever was rebuilt was never validated, so it cannot be trusted the
       way a cancelled sweep's partial result can.  Discard it. */
    glide_computer->RejectReconstruction(live_flight);
    TriggerCalculatedUpdate();
    TriggerMapUpdate();
    return;
  }

  LogFormat("Resume swept %u/%u fixes; replayed Session ends %s",
            fixes_processed, candidate->b_record_count,
            ends_airborne ? "airborne" : "on the ground");

  TriggerCalculatedUpdate();
  TriggerMapUpdate();

  if (fixes_processed == 0)
    return;

  /* The reconstruction is kept, and the file continued, only if the replayed
     Session ends in the air.  The case this catches: the pilot landed, the
     power was cut before landing detection fired, and they relaunched inside
     the staleness bound -- a winch or aerotow turnaround.  An unsigned file
     says "cut", but that Flight really had ended.

     This composes with cancellation without a special case.  A sweep stopped
     part-way is mid-Session, where the aircraft is airborne, so the check
     passes and the partial result is kept.  It only fails on an early cancel,
     before ten seconds of replayed movement, where there is nothing of value
     to discard. */
  if (!ends_airborne) {
    LogFormat("Cut Session ended on the ground; abandoning Resume");

    /* Nothing to undo in the file: the logger has not opened anything yet,
       so it will simply start a new one.  Only the computer state needs
       putting back. */
    glide_computer->RejectReconstruction(live_flight);

    Message::AddMessage(_("Previous flight had landed; not rebuilt"));

    TriggerCalculatedUpdate();
    TriggerMapUpdate();
    return;
  }

  /* Validated.  Now, and only now, is the logger allowed to continue that
     file -- AutoLogger start runs immediately after this handler. */
  logger->SetResumeTarget(candidate->path);

  if (fixes_processed < candidate->b_record_count)
    /* cancelled or out of time: what was rebuilt is kept, and the seam from
       there to the live position is handled by the same two-sample logic as
       the Gap seam */
    Message::AddMessage(_("Flight partly rebuilt"));
  else
    Message::AddMessage(_("Flight rebuilt"));
}
