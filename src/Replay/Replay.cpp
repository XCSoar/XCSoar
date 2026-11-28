// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay.hpp"
#include "IGC/IGCParser.hpp"
#include "IgcReplay.hpp"
#include "NmeaReplay.hpp"
#include "DemoReplayGlue.hpp"
#include "io/FileLineReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "CalculationThread.hpp"
#include "MergeThread.hpp"
#include "Logger/Logger.hpp"
#include "Device/Config.hpp"
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

#include <fmt/format.h>

#include <algorithm> // for std::clamp()
#include <cassert>
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

/* A midnight wrap is a short step once one day is added. A longer
   jump backward is a bad timestamp and stops the seek. Same bound as
   the IGC flight-time scan. */
constexpr FloatDuration ONE_DAY{24. * 60. * 60.};
constexpr FloatDuration HALF_DAY{12. * 60. * 60.};
constexpr FloatDuration MAX_MIDNIGHT_STEP{60. * 60.};

/**
 * Time of \a time on one timeline that continues across midnight.
 * \a new_offset is the day offset to keep for the next fix.
 * Undefined means a genuine backward jump.
 */
TimeStamp
ScanAbsoluteTime(TimeStamp time, TimeStamp last_raw,
                 FloatDuration day_offset,
                 FloatDuration &new_offset) noexcept
{
  new_offset = day_offset;

  const TimeStamp previous = last_raw + day_offset;
  const TimeStamp absolute = time + day_offset;
  if (!(absolute + HALF_DAY < previous)) {
    if (absolute < previous)
      return TimeStamp::Undefined();
    return absolute;
  }

  const TimeStamp rolled = absolute + ONE_DAY;
  const auto step = rolled - previous;
  if (step.count() > 0 && step <= MAX_MIDNIGHT_STEP) {
    new_offset = day_offset + ONE_DAY;
    return rolled;
  }

  return TimeStamp::Undefined();
}

TimeStamp
TimeOfDay(TimeStamp time) noexcept
{
  while (time.ToDuration() >= ONE_DAY)
    time = time - ONE_DAY;
  return time;
}

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
 * Show the scanned flight time in the progress dialog, in seconds.
 * Only for scans without a known target: where the range is known,
 * the bar itself already shows the amount and the time remaining.
 */
void
SetProgressText(OperationEnvironment &env, double elapsed_s) noexcept
{
  char buffer[16];
  const auto result = fmt::format_to_n(buffer, sizeof(buffer) - 1, "{} s",
                                       unsigned(std::max(elapsed_s, 0.)));
  *result.out = '\0';

  env.SetText(buffer);
}

/**
 * Run a seek scan through the #JobRunner.  The scan itself cannot
 * throw, but the runner may fail to show its progress dialog, and
 * #JobThread rethrows in the calling thread; the seek then keeps the
 * position it has reached, so seeks can be noexcept.
 */
void
RunSeekJob(JobRunner &runner, Job &job) noexcept
try {
  runner.Run(job);
} catch (...) {
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

  /* The fix time is ahead of this sample. Expire() treats a
     validity stamp in the future as a time warp and drops the
     logged airspeed. */
  if (data.airspeed_available)
    data.airspeed_available.Update(data.clock);

  return data;
}

} // namespace

void
Replay::Stop()
{
  if (replay == nullptr)
    return;

  if (seeking) {
    /* a seek is scanning the recording, possibly in a worker thread;
       deleting the AbstractReplay and the interpolator now would pull
       them away under the scan.  It polls this flag and stops, and
       the seek then calls Stop() again. */
    stop_pending = true;
    return;
  }

  stop_pending = false;

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
Replay::Start(Path _path, const DeviceConfig &device, bool keep_scan)
{
  assert(_path != nullptr);

  /* make sure the old AbstractReplay instance has cleaned up before
     creating a new one */
  Stop();

  device_config = &device;

  if (!keep_scan) {
    /* a public open may replace the file at the same path */
    recording_scan_valid = false;
    recording_scan = {};
  }

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
                            device);
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
Replay::PrepareRecordingScan(JobRunner &runner, bool &cancelled) noexcept
{
  cancelled = false;

  if (recording_scan_valid)
    return true;

  /* 0 done, 1 budget exhausted, 2 cancelled, 3 read failed */
  const auto read = [&](OperationEnvironment &env, bool bounded){
    const TimeStamp played_origin = recording_scan.start_time;
    recording_scan = {};

    if (path == nullptr || path.empty() ||
        !FilenameMatchesFileType(path.GetBase().c_str(), FileType::IGC)) {
      /* NMEA has no B records. Keep the first played fix as the
         origin, and do not retry the scan on the next seek. */
      recording_scan.start_time = played_origin;
      recording_scan_valid = true;
      return 0;
    }

    PeriodClock budget;
    budget.Update();

    try {
      FileLineReaderA reader{path};
      IGCRecordingSpan span;
      while (const char *line = reader.ReadLine()) {
        if (env.IsCancelled())
          return 2;

        if (bounded && budget.Check(SYNC_SEEK_BUDGET))
          /* shortcut: the job rereads from the start, resume the
             reader when a large IGC makes the repeated prefix matter */
          return 1;

        IGCScanBRecord(span, line);
      }

      recording_scan.fix_lines = span.fix_lines;
      recording_scan.start_time = span.start_time;
      recording_scan.end_time = span.end_time;
      recording_scan_valid = true;
      return 0;
    } catch (...) {
      /* leave the cache invalid so a later seek can retry */
      recording_scan = {};
      return 3;
    }
  };

  NullOperationEnvironment sync_env;
  const int sync_result = read(sync_env, true);
  if (sync_result == 0)
    return true;

  /* a failed read is not helped by showing the progress dialog */
  if (sync_result != 1)
    return false;

  ReplaySeekJob job{[&](OperationEnvironment &env){
    if (read(env, false) == 2)
      cancelled = true;
  }};
  RunSeekJob(runner, job);
  return recording_scan_valid;
}

bool
Replay::ReadNextFix(NMEAInfo &data) noexcept
try {
  if (!replay->Update(data))
    return false;

  ++fixes_read;
  return true;
} catch (...) {
  /* a broken or unreadable file ends the recording, just like
     reaching its end */
  return false;
}

bool
Replay::SeekToFlightElapsedMinutes(unsigned minutes,
                                    MergeThread &merge_thread,
                                    CalculationThread &calculation_thread,
                                    JobRunner &runner) noexcept
{
  if (minutes > MAX_SEEK_MINUTES)
    return false;

  /* an empty path means demo mode, which has no recording to seek
     in */
  if (!IsActive() || path == nullptr || path.empty())
    return false;

  bool scan_cancelled = false;
  if (!PrepareRecordingScan(runner, scan_cancelled))
    return scan_cancelled;

  const RecordingScan &scan = recording_scan;

  if (scan.start_time.IsDefined() && virtual_time.IsDefined()) {
    const TimeStamp forward_target =
      scan.start_time + FloatDuration{(double)minutes * 60.};

    if (forward_target == virtual_time)
      return true;

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

  if (device_config == nullptr)
    return false;

  try {
    Stop();
    Start(saved_path, *device_config, true);
  } catch (...) {
    return false;
  }

  timer.Cancel();

  /* declared before the suspension guard so the timer is re-armed
     after the worker threads have been resumed */
  AtScopeExit(this) {
    seeking = false;

    if (stop_pending)
      /* Stop() was called while the scan was running */
      Stop();
    else if (IsActive())
      timer.Schedule(std::chrono::milliseconds(100));
  };

  seeking = true;

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

  TimeStamp last_raw = anchor;
  FloatDuration day_offset{0};
  bool complete = false;
  bool warped = false;

  /* returns false when the time budget ran out and the scan must
     continue in a job behind the progress dialog */
  const auto run_scan = [&](OperationEnvironment &env, bool bounded){
    PeriodClock budget;
    budget.Update();

    PeriodClock progress_clock;

    while (true) {
      if (env.IsCancelled() || stop_pending)
        return true;

      if (bounded && budget.Check(SYNC_SEEK_BUDGET))
        return false;

      if (!next_data.time_available) {
        /* a fix without a time stamp, e.g. an NMEA sentence carrying
           none: skip it and keep scanning */
        if (!ReadNextFix(next_data))
          return true;

        continue;
      }

      FloatDuration new_offset{0};
      const TimeStamp absolute =
        ScanAbsoluteTime(next_data.time, last_raw, day_offset,
                         new_offset);
      if (!absolute.IsDefined()) {
        /* a genuine backward jump, not a midnight wrap */
        warped = true;
        return true;
      }

      if (absolute > target_ts)
        break;

      day_offset = new_offset;

      ApplyReplayFix(device_blackboard, merge_thread, calculation_thread,
                     next_data);
      last_raw = next_data.time;

      if (progress_clock.CheckUpdate(PROGRESS_UPDATE_INTERVAL)) {
        env.SetProgressRange(unsigned(progress_range));
        env.SetProgressPosition(
          unsigned(std::clamp((absolute - anchor).count(), 0.,
                              progress_range)));
      }

      if (cli != nullptr)
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

    RunSeekJob(runner, job);
  }

  /* when the seek stopped early (end of recording, time warp or
     cancelled), stay at the last applied fix instead of jumping past
     the applied data. The playback clock stays the time of day. */
  virtual_time = complete ? TimeOfDay(target_ts) : last_raw;

  if (complete && cli != nullptr) {
    /* the fix that ended the scan is still held back and has not
       reached the interpolator; feed it before reading past it, so
       the sample right after the target is not lost */
    if (next_data.time_available)
      cli->Update(next_data.time, next_data.location,
                  next_data.gps_altitude, next_data.pressure_altitude);

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

  return !warped;
}

bool
Replay::ForwardScan(MergeThread &merge_thread,
                    CalculationThread &calculation_thread,
                    JobRunner &runner,
                    std::function<bool(OperationEnvironment &)> matched,
                    bool &warped, CirclingMode *start_phase) noexcept
{
  timer.Cancel();
  warped = false;

  /* declared before the suspension guard so the timer is re-armed
     after the worker threads have been resumed */
  AtScopeExit(this) {
    seeking = false;

    if (stop_pending)
      /* Stop() was called while the scan was running */
      Stop();
    else if (IsActive())
      timer.Schedule(std::chrono::milliseconds(100));
  };

  seeking = true;

  const ScopedWorkerSuspend suspend{merge_thread, calculation_thread};

  /* the calculation thread can still publish a phase until it is
     suspended, so the seek's idea of "already in this phase" is read
     here, before the first fix is applied */
  if (start_phase != nullptr) {
    const std::lock_guard lock{device_blackboard.mutex};
    *start_phase = device_blackboard.Calculated().turn_mode;
  }

  bool found = false;
  bool cancelled = false;
  TimeStamp last_raw = virtual_time;
  FloatDuration day_offset{0};

  /* returns false when the time budget ran out and the scan must
     continue in a job behind the progress dialog */
  const auto run_scan = [&](OperationEnvironment &env, bool bounded){
    PeriodClock budget;
    budget.Update();

    while (!env.IsCancelled() && !stop_pending) {
      if (bounded && budget.Check(SYNC_SEEK_BUDGET))
        return false;

      if (!ReadNextFix(next_data))
        break;

      if (!next_data.time_available)
        continue;

      FloatDuration new_offset{0};
      const TimeStamp absolute =
        ScanAbsoluteTime(next_data.time, last_raw, day_offset,
                         new_offset);
      if (!absolute.IsDefined()) {
        /* a genuine backward jump, not a midnight wrap */
        warped = true;
        break;
      }

      day_offset = new_offset;

      ApplyReplayFix(device_blackboard, merge_thread, calculation_thread,
                     next_data);
      last_raw = next_data.time;

      if (cli != nullptr)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude, next_data.pressure_altitude);

      if (matched(env)) {
        found = true;
        break;
      }
    }

    /* a cancel is not a missing phase: the caller keeps the position
       reached so far and should not report a failed search */
    if (env.IsCancelled() || stop_pending)
      cancelled = true;

    return true;
  };

  /* scan synchronously first: short seeks finish without flashing a
     progress dialog, only long ones escalate to the runner */
  NullOperationEnvironment sync_env;
  if (!run_scan(sync_env, true)) {
    ReplaySeekJob job{[&](OperationEnvironment &env){
      run_scan(env, false);
    }};

    RunSeekJob(runner, job);
  }

  /* even when no match was found (end of recording, time warp or
     cancelled), keep the position that was reached, so playback
     resumes consistently with the fixes already applied. The clock
     stays the time of day, including after midnight. Update it even
     when the position did not move, so the time spent seeking is not
     played back. */
  clock.Update();

  if (last_raw != virtual_time || day_offset.count() != 0) {
    virtual_time = last_raw;
    fast_forward = TimeStamp::Undefined();

    {
      const std::lock_guard lock{device_blackboard.mutex};
      CommonInterface::ReadBlackboardBasic(device_blackboard.Basic());
    }

    TriggerCalculatedUpdate();
    TriggerVarioUpdate();
  }

  return found || cancelled;
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
     error; a genuine backward jump is */
  bool warped = false;
  const bool reached =
    ForwardScan(merge_thread, calculation_thread, runner,
                [this, start, target_ts, progress_range,
                 day_offset = FloatDuration{0},
                 last_raw = start,
                 progress_clock = PeriodClock{}]
                (OperationEnvironment &env) mutable {
    FloatDuration new_offset{0};
    const TimeStamp absolute =
      ScanAbsoluteTime(next_data.time, last_raw, day_offset, new_offset);
    if (absolute.IsDefined()) {
      day_offset = new_offset;
      last_raw = next_data.time;
    }

    if (progress_clock.CheckUpdate(PROGRESS_UPDATE_INTERVAL)) {
      env.SetProgressRange(unsigned(progress_range));

      const TimeStamp shown = absolute.IsDefined() ? absolute : start;
      const auto elapsed = shown - start;
      env.SetProgressPosition(
        unsigned(std::clamp(elapsed.count(), 0., progress_range)));
    }

    return absolute.IsDefined() && absolute >= target_ts;
  }, warped);

  return reached || !warped;
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
  bool scan_cancelled = false;
  if (!PrepareRecordingScan(runner, scan_cancelled))
    return scan_cancelled;

  const RecordingScan &scan = recording_scan;
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
  bool scan_cancelled = false;
  if (!PrepareRecordingScan(runner, scan_cancelled))
    return scan_cancelled;

  const unsigned total_fixes = recording_scan.fix_lines;
  const unsigned remaining_fixes =
    total_fixes > fixes_read ? total_fixes - fixes_read : 0;

  /* filled by ForwardScan after the workers are suspended, before
     the first fix of this seek is applied */
  CirclingMode start_mode{};
  const TimeStamp scan_start = virtual_time;

  bool warped = false;
  return ForwardScan(merge_thread, calculation_thread, runner,
                     [this, mode, &start_mode,
                      passed_current_phase = false,
                      have_start = false,
                      remaining_fixes, scan_start, applied = 0u,
                      progress_clock = PeriodClock{}]
                     (OperationEnvironment &env) mutable {
    if (!have_start) {
      passed_current_phase = !IsInPhase(mode, start_mode);
      have_start = true;
    }

    ++applied;

    if (progress_clock.CheckUpdate(PROGRESS_UPDATE_INTERVAL)) {
      if (remaining_fixes > 0) {
        env.SetProgressRange(remaining_fixes);
        env.SetProgressPosition(std::min(applied, remaining_fixes));
      }

      SetProgressText(env, (next_data.time - scan_start).count());
    }

    const auto current = device_blackboard.Calculated().turn_mode;
    if (!passed_current_phase && !IsInPhase(mode, current))
      passed_current_phase = true;

    return passed_current_phase && current == mode;
  }, warped, &start_mode);
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
          if (!recording_scan.start_time.IsDefined())
            recording_scan.start_time = next_data.time;
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
      if (!recording_scan.start_time.IsDefined())
        recording_scan.start_time = virtual_time;
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

    if (data.time_available) {
      if (!recording_scan.start_time.IsDefined())
        recording_scan.start_time = data.time;
      virtual_time = data.time;
    }

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
  else if (cli != nullptr) {
    /* Interpolated IGC emits one GPS sample per timer tick, with
       virtual time advancing by elapsed × rate.  A fixed 1 s wall
       timer therefore produces 10 s flight steps at 10×, which is too
       sparse for circling wind.  Keep flight-time steps near 1 Hz. */
    const double scale = std::max(time_scale, 0.1);
    constexpr std::chrono::steady_clock::duration lower =
      std::chrono::milliseconds(50);
    constexpr std::chrono::steady_clock::duration upper =
      std::chrono::seconds(1);
    const auto period =
      std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        FloatDuration{1} / scale);
    schedule = std::clamp(period, lower, upper);
  } else {
    constexpr std::chrono::steady_clock::duration lower = std::chrono::milliseconds(100);
    constexpr std::chrono::steady_clock::duration upper = std::chrono::seconds(3);
    const FloatDuration delta_s((next_data.time - virtual_time) / time_scale);
    const auto delta = std::chrono::duration_cast<std::chrono::steady_clock::duration>(delta_s);
    schedule = std::clamp(delta, lower, upper);
  }

  timer.Schedule(schedule);
}
