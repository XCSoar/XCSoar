// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/event/Timer.hpp"
#include "NMEA/Info.hpp"
#include "time/PeriodClock.hpp"
#include "time/Stamp.hpp"
#include "system/Path.hpp"

class DeviceBlackboard;
class Logger;
class ProtectedTaskManager;
class AbstractReplay;
class CatmullRomInterpolator;
class MergeThread;
class CalculationThread;
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

private:
  /**
   * Open the file and reset the replay state, without scheduling the paced
   * timer.
   *
   * Throws std::runtime_errror on error.
   */
  void Open(Path _path);

public:
  void Stop();

  /**
   * Throws std::runtime_errror on error.
   */
  void Start(Path _path);

  /**
   * Like Start(), but for a Resume sweep, which consumes the whole file at
   * once and so wants no paced timer running alongside it.
   *
   * Call on the UI thread: UI::Timer is not thread-safe, and Stop() cancels
   * one.
   *
   * Throws std::runtime_errror on error.
   */
  void StartSweep(Path _path);

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
   * Like ProcessAllFixes(), but reports progress and stops early when \a env
   * is cancelled, for the Resume sweep behind its progress dialog.
   *
   * Unlike ProcessAllFixes() this touches no UI::Timer, so it is safe to run
   * on a worker thread -- provided the caller opened the file with
   * StartSweep() on the UI thread, so no paced replay is running.
   *
   * \a merge_thread and \a calc_thread must be suspended.
   *
   * @param total the expected number of fixes, for the progress range
   * @return the number of fixes processed, which is fewer than \a total if
   * the sweep was cancelled -- the partial result is kept either way
   */
  unsigned SweepAllFixes(MergeThread &merge_thread,
                         CalculationThread &calc_thread,
                         OperationEnvironment &env, unsigned total);

private:
  void OnTimer();
};
