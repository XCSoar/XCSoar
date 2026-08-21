// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/event/Timer.hpp"
#include "NMEA/Info.hpp"
#include "time/PeriodClock.hpp"
#include "time/Stamp.hpp"
#include "system/Path.hpp"

#include "NMEA/CirclingInfo.hpp"

#include <functional>

class DeviceBlackboard;
class Logger;
class ProtectedTaskManager;
class AbstractReplay;
class CatmullRomInterpolator;
class MergeThread;
class CalculationThread;
class JobRunner;
class OperationEnvironment;
class Error;

class Replay final
{
  DeviceBlackboard &device_blackboard;

  UI::Timer timer{[this]{ OnTimer(); }};

  double time_scale = 1;

  AbstractReplay *replay = nullptr;

  Logger *const logger;
  ProtectedTaskManager &task_manager;

  AllocatedPath path = nullptr;

  /**
   * The time of day according to replay input.  This is negative if
   * unknown.
   */
  TimeStamp virtual_time;

  /**
   * If this value is not negative, then we're in fast-forward mode:
   * replay is going as quickly as possible.  This value denotes the
   * time stamp when we will stop going fast-forward.  If
   * #virtual_time is negative, then this is the duration, and
   * #virtual_time will be added as soon as it is known.
   */
  TimeStamp fast_forward;

  /**
   * Keeps track of the wall-clock time between two Update() calls.
   */
  PeriodClock clock;

  /**
   * The last NMEAInfo returned by the #AbstractReplay instance.  It
   * is held back until #virtual_time has passed #next_data.time.
   */
  NMEAInfo next_data;

  /**
   * The number of fixes read from the #AbstractReplay since Start().
   * Used to scale the seek progress bar.
   */
  unsigned fixes_read;

  CatmullRomInterpolator *cli = nullptr;

public:
  Replay(DeviceBlackboard &_device_blackboard,
         Logger *_logger, ProtectedTaskManager &_task_manager)
    :device_blackboard(_device_blackboard),
     logger(_logger), task_manager(_task_manager) {}

  ~Replay() {
    Stop();
  }

  bool IsActive() const {
    return replay != nullptr;
  }

private:
  bool Update();

public:
  void Stop();

  /**
   * Throws std::runtime_errror on error.
   */
  void Start(Path _path);

  Path GetFilename() const {
    return path;
  }

  double GetTimeScale() const {
    return time_scale;
  }

  void SetTimeScale(const double _time_scale) {
    time_scale = _time_scale;
  }

  /**
   * Start fast-forwarding the replay by the specified number of
   * seconds.  This replays the given amount of time from the input
   * time as quickly as possible.  Returns false if unable to fast forward.
   */
  bool FastForward(FloatDuration delta_s) noexcept {
    if (!IsActive())
      return false;

    if (virtual_time.IsDefined()) {
      fast_forward = virtual_time + delta_s;
      return true;
    } else {
      fast_forward = TimeStamp{delta_s};
      return false;
    }
  }

  TimeStamp GetVirtualTime() const noexcept {
    return virtual_time;
  }

  /**
   * Feed every fix from the current replay file through merge and
   * calculation without virtual-time skipping.  For trail testing.
   * Returns the number of fixes processed (0 if replay is inactive or
   * demo mode).  \a merge_thread and \a calc_thread must be suspended.
   */
  unsigned ProcessAllFixes(MergeThread &merge_thread,
                           CalculationThread &calc_thread);

  /**
   * Restart the current recording and replay up to the given number
   * of minutes after the first fix, merging every fix into the
   * blackboard.  The scan is run through the given #JobRunner, which
   * may report progress and allow cancelling; a cancelled seek keeps
   * the position that was reached.
   */
  bool SeekToFlightElapsedMinutes(unsigned minutes,
                                  MergeThread &merge_thread,
                                  CalculationThread &calculation_thread,
                                  JobRunner &runner) noexcept;

  /**
   * Replay forward from the current position until the flight mode
   * changes to the requested state (circling or cruise).  The scan is
   * run through the given #JobRunner, which may allow cancelling; a
   * cancelled seek keeps the position that was reached.
   */
  bool SeekToNextFlightMode(CirclingMode mode,
                            MergeThread &merge_thread,
                            CalculationThread &calculation_thread,
                            JobRunner &runner) noexcept;

private:
  /**
   * Read the next fix from the #AbstractReplay, keeping #fixes_read
   * up to date.
   */
  bool ReadNextFix(NMEAInfo &data);

  /**
   * Apply fixes forward from the current position through merge and
   * calculation until the given predicate matches the state after a
   * fix, the recording ends, time warps or the scan is cancelled.
   * The worker threads are suspended while scanning, and the reached
   * position becomes the new playback position.  Call only while the
   * replay is active with a defined #virtual_time and not in demo
   * mode.
   *
   * @return true if the predicate matched
   */
  bool ForwardScan(MergeThread &merge_thread,
                   CalculationThread &calculation_thread,
                   JobRunner &runner,
                   std::function<bool(OperationEnvironment &)> matched) noexcept;

  /**
   * Scan forward from the current position until the given time of
   * day, showing progress scaled to \a progress_end (usually the
   * target, clamped at the recording end).  Reaching the end of the
   * recording before the target is not an error.
   */
  bool ForwardScanToTime(TimeStamp target_ts, TimeStamp progress_end,
                         MergeThread &merge_thread,
                         CalculationThread &calculation_thread,
                         JobRunner &runner) noexcept;

  void OnTimer();
};
