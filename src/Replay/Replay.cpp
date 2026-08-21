// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay.hpp"
#include "IgcReplay.hpp"
#include "NmeaReplay.hpp"
#include "DemoReplayGlue.hpp"
#include "io/FileLineReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "CalculationThread.hpp"
#include "MergeThread.hpp"
#include "Logger/Logger.hpp"
#include "Interface.hpp"
#include "Repository/FileType.hpp"
#include "CatmullRomInterpolator.hpp"
#include "Geo/GeoVector.hpp"
#include "Protection.hpp"
#include "Job/Job.hpp"
#include "Job/Runner.hpp"
#include "Operation/Operation.hpp"
#include "time/Cast.hxx"
#include "util/ScopeExit.hxx"

#include <algorithm> // for std::clamp()
#include <cassert>
#include <cstdio>
#include <functional>
#include <stdexcept>

namespace {

/**
 * How long a seek may scan synchronously on the UI thread before it
 * escalates to a progress dialog via the #JobRunner.  Short scans
 * finish inline, so no dialog flashes up just to disappear again.
 */
constexpr std::chrono::steady_clock::duration SYNC_SEEK_BUDGET =
  std::chrono::milliseconds(500);

/**
 * Minimum interval between seek progress updates.  Posting on every
 * fix would flood the event queue faster than the UI thread drains
 * it, so updates are rate-limited at the source.
 */
constexpr std::chrono::steady_clock::duration PROGRESS_UPDATE_INTERVAL =
  std::chrono::milliseconds(100);

void
ApplyReplayFix(DeviceBlackboard &device_blackboard,
               MergeThread &merge_thread,
               CalculationThread &calculation_thread,
               const NMEAInfo &fix) noexcept
{
  {
    const std::lock_guard lock{device_blackboard.mutex};
    device_blackboard.SetReplayState() = fix;
  }

  merge_thread.ProcessReplayFix();
  calculation_thread.ProcessReplayFix();
}

/**
 * Result of a cheap parse-only pass over the recording, without merge
 * or calculation.  Used to scale the seek progress bar.
 */
struct RecordingScan {
  /**
   * The number of lines that will produce replay fixes (IGC B
   * records); 0 if unknown for this file type.
   */
  unsigned fix_lines = 0;

  /**
   * The time of day of the first fix, or undefined if unknown.
   */
  TimeStamp start_time = TimeStamp::Undefined();

  /**
   * The time of day of the last fix, or undefined if unknown.
   */
  TimeStamp end_time = TimeStamp::Undefined();
};

RecordingScan
ScanRecording(Path path) noexcept
try {
  RecordingScan result;

  if (path == nullptr || path.empty() ||
      !FilenameMatchesFileType(path.GetBase().c_str(), FileType::IGC))
    return result;

  FileLineReaderA reader{path};
  while (const char *line = reader.ReadLine()) {
    if (*line != 'B')
      continue;

    ++result.fix_lines;

    /* B records start with the time of day as HHMMSS; the comparison
       stops at the terminating null byte of short lines */
    const char *t = line + 1;
    bool valid = true;
    for (unsigned i = 0; valid && i < 6; ++i)
      valid = t[i] >= '0' && t[i] <= '9';

    if (valid) {
      const unsigned hh = (t[0] - '0') * 10 + (t[1] - '0');
      const unsigned mm = (t[2] - '0') * 10 + (t[3] - '0');
      const unsigned ss = (t[4] - '0') * 10 + (t[5] - '0');
      if (hh < 24 && mm < 60 && ss < 60) {
        const TimeStamp time_stamp{
          std::chrono::seconds{hh * 3600 + mm * 60 + ss}};
        if (!result.start_time.IsDefined())
          result.start_time = time_stamp;
        result.end_time = time_stamp;
      }
    }
  }

  return result;
} catch (...) {
  return {};
}

/**
 * Show the scanned flight time in the progress dialog, in seconds,
 * as "1234 / 12000 s" when the total is known and "1234 s"
 * otherwise.
 */
void
SetProgressText(OperationEnvironment &env,
                double elapsed_s, double total_s) noexcept
{
  char buffer[32];
  if (total_s > 0)
    snprintf(buffer, sizeof(buffer), "%u / %u s",
             unsigned(std::clamp(elapsed_s, 0., total_s)),
             unsigned(total_s));
  else
    snprintf(buffer, sizeof(buffer), "%u s",
             unsigned(std::max(elapsed_s, 0.)));

  env.SetText(buffer);
}

/**
 * Adapter running a seek scan through a #JobRunner, so the potentially
 * expensive loop can be offloaded into a worker thread with progress
 * reporting and cancellation.
 */
class ReplaySeekJob final : public Job {
  std::function<void(OperationEnvironment &)> function;

public:
  template<typename F>
  explicit ReplaySeekJob(F &&_function) noexcept
    :function(std::forward<F>(_function)) {}

  void Run(OperationEnvironment &env) override {
    function(env);
  }
};

/**
 * Does the given turn mode belong to the given definite flight phase
 * (#CirclingMode::CLIMB or #CirclingMode::CRUISE)?  The transitional
 * POSSIBLE_* states count as part of the phase they may be leaving,
 * because they often revert without an actual phase change.
 */
constexpr bool
IsInPhase(CirclingMode phase, CirclingMode mode) noexcept
{
  return phase == CirclingMode::CLIMB
    ? mode == CirclingMode::CLIMB || mode == CirclingMode::POSSIBLE_CRUISE
    : mode == CirclingMode::CRUISE || mode == CirclingMode::POSSIBLE_CLIMB;
}

/**
 * Suspend the merge and calculation worker threads for the lifetime
 * of this object so replay fixes can be processed synchronously.
 */
struct ScopedWorkerSuspend final {
  MergeThread &merge_thread;
  CalculationThread &calculation_thread;

  ScopedWorkerSuspend(MergeThread &m, CalculationThread &c) noexcept
    :merge_thread(m), calculation_thread(c)
  {
    calculation_thread.Suspend();
    merge_thread.Suspend();
  }

  ~ScopedWorkerSuspend() noexcept
  {
    merge_thread.Resume();
    calculation_thread.Resume();
  }

  ScopedWorkerSuspend(const ScopedWorkerSuspend &) = delete;
  ScopedWorkerSuspend &operator=(const ScopedWorkerSuspend &) = delete;
};

/**
 * Build a replay fix from the Catmull-Rom interpolator at the given
 * time stamp, copying all other fields from the last replay fix.
 */
NMEAInfo
MakeInterpolatedFix(const CatmullRomInterpolator &cli, TimeStamp time,
                    const NMEAInfo &last) noexcept
{
  const CatmullRomInterpolator::Record r = cli.Interpolate(time);
  const GeoVector v = cli.GetVector(time);

  NMEAInfo data = last;
  data.clock = time;
  data.alive.Update(data.clock);
  data.ProvideTime(time);
  data.location = r.location;
  data.location_available.Update(data.clock);
  data.ground_speed = v.distance;
  data.ground_speed_available.Update(data.clock);
  data.track = v.bearing;
  data.track_available.Update(data.clock);
  data.gps_altitude = r.gps_altitude;
  data.gps_altitude_available.Update(data.clock);
  data.ProvidePressureAltitude(r.baro_altitude);
  data.ProvideBaroAltitudeTrue(r.baro_altitude);
  return data;
}

} // namespace

void
Replay::Stop()
{
  if (replay == nullptr)
    return;

  timer.Cancel();

  delete replay;
  replay = nullptr;

  delete cli;
  cli = nullptr;

  device_blackboard.StopReplay();

  if (logger != nullptr)
    logger->ClearBuffer();
}

void
Replay::Start(Path _path)
{
  assert(_path != nullptr);

  /* make sure the old AbstractReplay instance has cleaned up before
     creating a new one */
  Stop();

  path = _path;

  if (path == nullptr || path.empty()) {
    replay = new DemoReplayGlue(device_blackboard, task_manager);
  } else if (FilenameMatchesFileType(path.GetBase().c_str(),
                                      FileType::IGC)) {
    replay = new IgcReplay(std::make_unique<FileLineReaderA>(path));

    cli = new CatmullRomInterpolator(FloatDuration{0.98});
    cli->Reset();
  } else {
    replay = new NmeaReplay(std::make_unique<FileLineReaderA>(path),
                            CommonInterface::GetSystemSettings().devices[0]);
  }

  if (logger != nullptr)
    logger->ClearBuffer();

  virtual_time = TimeStamp::Undefined();
  fast_forward = TimeStamp::Undefined();
  next_data.Reset();
  fixes_read = 0;

  timer.Schedule(std::chrono::milliseconds(100));
}

bool
Replay::ReadNextFix(NMEAInfo &data)
{
  if (!replay->Update(data))
    return false;

  ++fixes_read;
  return true;
}

bool
Replay::SeekToFlightElapsedMinutes(unsigned minutes,
                                    MergeThread &merge_thread,
                                    CalculationThread &calculation_thread,
                                    JobRunner &runner) noexcept
{
  constexpr unsigned MAX_MINUTES = 24 * 60;
  if (minutes > MAX_MINUTES)
    return false;

  /* an empty path means demo mode, which has no recording to seek
     in */
  if (!IsActive() || path == nullptr || path.empty())
    return false;

  const RecordingScan scan = ScanRecording(path);

  if (scan.start_time.IsDefined() && virtual_time.IsDefined()) {
    const TimeStamp forward_target =
      scan.start_time + FloatDuration{(double)minutes * 60.};

    if (forward_target > virtual_time) {
      /* the target lies ahead of the current position: scan forward
         instead of restarting and replaying from the beginning */
      TimeStamp progress_end = forward_target;
      if (scan.end_time.IsDefined() && scan.end_time > virtual_time &&
          scan.end_time < forward_target)
        progress_end = scan.end_time;

      return ForwardScanToTime(forward_target, progress_end,
                               merge_thread, calculation_thread, runner);
    }
  }

  timer.Cancel();

  const AllocatedPath saved_path{GetFilename()};

  try {
    Stop();
    Start(saved_path);
  } catch (...) {
    return false;
  }

  timer.Cancel();

  /* declared before the suspension guard so the timer is re-armed
     after the worker threads have been resumed */
  AtScopeExit(this) {
    if (IsActive())
      timer.Schedule(std::chrono::milliseconds(100));
  };

  const ScopedWorkerSuspend suspend{merge_thread, calculation_thread};

  /* the derived state (trail, statistics) of the previous playback
     would otherwise be mixed with the fixes applied again from the
     start of the recording */
  calculation_thread.ResetFlight();

  next_data.Reset();

  while (!next_data.time_available) {
    if (!ReadNextFix(next_data))
      return false;
  }

  const TimeStamp anchor = next_data.time;
  const TimeStamp target_ts =
    anchor + FloatDuration{(double)minutes * 60.};

  /* clamp the progress scale at the recording end, so the bar still
     reaches 100% when the requested minute lies beyond the flight */
  TimeStamp progress_end = target_ts;
  if (scan.end_time.IsDefined() && scan.end_time > anchor &&
      scan.end_time < target_ts)
    progress_end = scan.end_time;

  const double progress_range =
    std::max(1., (progress_end - anchor).count());

  TimeStamp last_time = anchor;
  bool complete = false;

  /* returns false when the time budget ran out and the scan must
     continue in a job behind the progress dialog */
  const auto run_scan = [&](OperationEnvironment &env, bool bounded){
    PeriodClock budget;
    budget.Update();

    PeriodClock progress_clock;

    while (next_data.time_available && next_data.time <= target_ts) {
      if (env.IsCancelled())
        return true;

      if (bounded && budget.Check(SYNC_SEEK_BUDGET))
        return false;

      if (next_data.time < last_time)
        /* time warp, e.g. midnight wraparound during NMEA replay:
           stop here instead of replaying the whole remaining file */
        return true;

      ApplyReplayFix(device_blackboard, merge_thread, calculation_thread,
                     next_data);
      last_time = next_data.time;

      if (progress_clock.CheckUpdate(PROGRESS_UPDATE_INTERVAL)) {
        env.SetProgressRange(unsigned(progress_range));
        env.SetProgressPosition(
          unsigned(std::clamp((last_time - anchor).count(), 0.,
                              progress_range)));
        SetProgressText(env, (last_time - anchor).count(), progress_range);
      }

      if (cli != nullptr && next_data.time_available)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude, next_data.pressure_altitude);

      if (!ReadNextFix(next_data))
        return true;
    }

    complete = true;
    return true;
  };

  NullOperationEnvironment sync_env;
  if (!run_scan(sync_env, true)) {
    ReplaySeekJob job{[&](OperationEnvironment &env){
      run_scan(env, false);
    }};

    runner.Run(job);
  }

  /* when the seek stopped early (end of recording, time warp or
     cancelled), stay at the last applied fix instead of jumping past
     the applied data */
  virtual_time = complete ? target_ts : last_time;

  if (complete && cli != nullptr) {
    while (cli->NeedData(virtual_time)) {
      if (!ReadNextFix(next_data))
        break;

      if (next_data.time_available)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude, next_data.pressure_altitude);
    }

    if (cli->Ready())
      ApplyReplayFix(device_blackboard, merge_thread, calculation_thread,
                     MakeInterpolatedFix(*cli, virtual_time, next_data));
  }

  fast_forward = TimeStamp::Undefined();
  clock.Update();

  {
    const std::lock_guard lock{device_blackboard.mutex};
    CommonInterface::ReadBlackboardBasic(device_blackboard.Basic());
  }

  TriggerCalculatedUpdate();
  TriggerVarioUpdate();

  return true;
}

bool
Replay::ForwardScan(MergeThread &merge_thread,
                    CalculationThread &calculation_thread,
                    JobRunner &runner,
                    std::function<bool(OperationEnvironment &)> matched) noexcept
{
  timer.Cancel();

  /* declared before the suspension guard so the timer is re-armed
     after the worker threads have been resumed */
  AtScopeExit(this) {
    if (IsActive())
      timer.Schedule(std::chrono::milliseconds(100));
  };

  const ScopedWorkerSuspend suspend{merge_thread, calculation_thread};

  bool found = false;
  TimeStamp last_time = virtual_time;

  /* returns false when the time budget ran out and the scan must
     continue in a job behind the progress dialog */
  const auto run_scan = [&](OperationEnvironment &env, bool bounded){
    PeriodClock budget;
    budget.Update();

    while (!env.IsCancelled()) {
      if (bounded && budget.Check(SYNC_SEEK_BUDGET))
        return false;

      if (!ReadNextFix(next_data))
        break;

      if (!next_data.time_available)
        continue;

      if (next_data.time < last_time)
        /* time warp, e.g. midnight wraparound during NMEA replay */
        break;

      ApplyReplayFix(device_blackboard, merge_thread, calculation_thread,
                     next_data);
      last_time = next_data.time;

      if (cli != nullptr)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude, next_data.pressure_altitude);

      if (matched(env)) {
        found = true;
        break;
      }
    }

    return true;
  };

  /* scan synchronously first: short seeks finish without flashing a
     progress dialog, only long ones escalate to the runner */
  NullOperationEnvironment sync_env;
  if (!run_scan(sync_env, true)) {
    ReplaySeekJob job{[&](OperationEnvironment &env){
      run_scan(env, false);
    }};

    runner.Run(job);
  }

  /* even when no match was found (end of recording, time warp or
     cancelled), keep the position that was reached, so playback
     resumes consistently with the fixes already applied */
  if (last_time > virtual_time) {
    virtual_time = last_time;
    fast_forward = TimeStamp::Undefined();
    clock.Update();

    {
      const std::lock_guard lock{device_blackboard.mutex};
      CommonInterface::ReadBlackboardBasic(device_blackboard.Basic());
    }

    TriggerCalculatedUpdate();
    TriggerVarioUpdate();
  }

  return found;
}

bool
Replay::ForwardScanToTime(TimeStamp target_ts, TimeStamp progress_end,
                          MergeThread &merge_thread,
                          CalculationThread &calculation_thread,
                          JobRunner &runner) noexcept
{
  const TimeStamp start = virtual_time;
  const double progress_range =
    std::max(1., (progress_end - start).count());

  /* reaching the end of the recording before the target is not an
     error, so the scan result is ignored */
  ForwardScan(merge_thread, calculation_thread, runner,
              [this, start, target_ts, progress_range,
               progress_clock = PeriodClock{}]
              (OperationEnvironment &env) mutable {
    if (progress_clock.CheckUpdate(PROGRESS_UPDATE_INTERVAL)) {
      env.SetProgressRange(unsigned(progress_range));

      const auto elapsed = next_data.time - start;
      env.SetProgressPosition(
        unsigned(std::clamp(elapsed.count(), 0., progress_range)));
      SetProgressText(env, elapsed.count(), progress_range);
    }

    return next_data.time >= target_ts;
  });

  return true;
}

bool
Replay::SeekForward(FloatDuration delta,
                    MergeThread &merge_thread,
                    CalculationThread &calculation_thread,
                    JobRunner &runner) noexcept
{
  /* an empty path means demo mode, which has no recording to seek
     in */
  if (!IsActive() || path == nullptr || path.empty() ||
      !virtual_time.IsDefined() || delta.count() <= 0)
    return false;

  const TimeStamp target_ts = virtual_time + delta;

  /* clamp the progress scale at the recording end, so the bar still
     reaches 100% when skipping past the end of the flight */
  const RecordingScan scan = ScanRecording(path);
  TimeStamp progress_end = target_ts;
  if (scan.end_time.IsDefined() && scan.end_time > virtual_time &&
      scan.end_time < target_ts)
    progress_end = scan.end_time;

  return ForwardScanToTime(target_ts, progress_end,
                           merge_thread, calculation_thread, runner);
}

bool
Replay::SeekToNextFlightMode(CirclingMode mode,
                              MergeThread &merge_thread,
                              CalculationThread &calculation_thread,
                              JobRunner &runner) noexcept
{
  /* an empty path means demo mode, which has no recording to seek
     in */
  if (!IsActive() || path == nullptr || path.empty() ||
      !virtual_time.IsDefined())
    return false;

  /* the number of fixes until the requested phase cannot be known in
     advance, so the bar shows how far through the remaining recording
     the scan has come */
  const unsigned total_fixes = ScanRecording(path).fix_lines;
  const unsigned remaining_fixes =
    total_fixes > fixes_read ? total_fixes - fixes_read : 0;

  bool passed_current_phase =
    !IsInPhase(mode, device_blackboard.Calculated().turn_mode);
  const TimeStamp scan_start = virtual_time;

  return ForwardScan(merge_thread, calculation_thread, runner,
                     [this, mode, passed_current_phase,
                      remaining_fixes, scan_start, applied = 0u,
                      progress_clock = PeriodClock{}]
                     (OperationEnvironment &env) mutable {
    ++applied;

    if (progress_clock.CheckUpdate(PROGRESS_UPDATE_INTERVAL)) {
      if (remaining_fixes > 0) {
        env.SetProgressRange(remaining_fixes);
        env.SetProgressPosition(std::min(applied, remaining_fixes));
      }

      SetProgressText(env, (next_data.time - scan_start).count(), 0);
    }

    const auto current = device_blackboard.Calculated().turn_mode;
    if (!passed_current_phase && !IsInPhase(mode, current))
      passed_current_phase = true;

    return passed_current_phase && current == mode;
  });
}

bool
Replay::Update()
{
  if (replay == nullptr)
    return false;

  if (time_scale <= 0) {
    /* replay is paused */
    /* to avoid a big fast-forward with the next
       PeriodClock::ElapsedUpdate() call below after unpausing, update
       the clock each time we're called while paused */
    clock.Update();
    return true;
  }

  const auto old_virtual_time = virtual_time;

  if (virtual_time.IsDefined()) {
    /* update the virtual time */
    assert(clock.IsDefined());

    if (!fast_forward.IsDefined()) {
      virtual_time += clock.ElapsedUpdate() * time_scale;
    } else {
      clock.Update();

      virtual_time += std::chrono::seconds{1};
      if (virtual_time >= fast_forward)
        fast_forward = TimeStamp::Undefined();
    }
  } else {
    /* if we ever received a valid time from the AbstractReplay, then
       virtual_time must be initialised */
    assert(!next_data.time_available);
  }

  if (cli == nullptr || fast_forward.IsDefined()) {
    if (next_data.time_available && virtual_time < next_data.time)
      /* still not time to use next_data */
      return true;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      device_blackboard.SetReplayState() = next_data;
      device_blackboard.ScheduleMerge();
    }

    while (true) {
      if (!ReadNextFix(next_data)) {
        Stop();
        return false;
      }

      assert(!next_data.gps.real);

      if (next_data.time_available) {
        if (!virtual_time.IsDefined()) {
          virtual_time = next_data.time;
          if (fast_forward.IsDefined())
            fast_forward = virtual_time + fast_forward.ToDuration();
          clock.Update();
          break;
        }

        if (next_data.time >= virtual_time)
          break;

        if (next_data.time < old_virtual_time) {
          /* time warp; that can happen on midnight wraparound during
             NMEA replay */
          virtual_time = next_data.time;
          break;
        }
      }
    }
  } else {
    while (cli->NeedData(virtual_time)) {
      if (!ReadNextFix(next_data)) {
        Stop();
        return false;
      }

      assert(!next_data.gps.real);

      if (next_data.time_available)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude,
                    next_data.pressure_altitude);
    }

    if (!virtual_time.IsDefined()) {
      virtual_time = cli->GetMaxTime();
      if (fast_forward.IsDefined())
        fast_forward = virtual_time + fast_forward.ToDuration();
      clock.Update();
    }

    const NMEAInfo data = MakeInterpolatedFix(*cli, virtual_time, next_data);

    {
      const std::lock_guard lock{device_blackboard.mutex};
      device_blackboard.SetReplayState() = data;
      device_blackboard.ScheduleMerge();
    }
  }

  return true;
}

unsigned
Replay::ProcessAllFixes(MergeThread &merge_thread,
                        CalculationThread &calc_thread)
{
  if (replay == nullptr || path == nullptr || path.empty())
    return 0;

  timer.Cancel();
  fast_forward = TimeStamp::Undefined();

  NMEAInfo data;
  data.Reset();
  unsigned count = 0;

  while (ReadNextFix(data)) {
    assert(!data.gps.real);

    if (data.time_available)
      virtual_time = data.time;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      device_blackboard.SetReplayState() = data;
    }

    merge_thread.ProcessReplayFix();
    calc_thread.ProcessReplayFix();
    ++count;

    if (data.time_available)
      data.Expire();
  }

  if (count > 0)
    next_data = data;

  return count;
}

void
Replay::OnTimer()
{
  if (!Update())
    return;

  std::chrono::steady_clock::duration schedule;
  if (time_scale <= 0)
    schedule = std::chrono::seconds(1);
  else if (fast_forward.IsDefined())
    schedule = std::chrono::milliseconds(100);
  else if (!virtual_time.IsDefined() || !next_data.time_available)
    schedule = std::chrono::milliseconds(500);
  else if (cli != nullptr)
    schedule = std::chrono::seconds(1);
  else {
    constexpr std::chrono::steady_clock::duration lower = std::chrono::milliseconds(100);
    constexpr std::chrono::steady_clock::duration upper = std::chrono::seconds(3);
    const FloatDuration delta_s((next_data.time - virtual_time) / time_scale);
    const auto delta = std::chrono::duration_cast<std::chrono::steady_clock::duration>(delta_s);
    schedule = std::clamp(delta, lower, upper);
  }

  timer.Schedule(schedule);
}
