// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

#include <cstdint>

enum class ClimbSampleAction : std::uint8_t {
  IGNORED,
  APPENDED,
  REPLACED,
};

struct ClimbSamplePolicy {
  FloatDuration minimum_interval;
  FloatDuration maximum_gap;
};

struct ClimbAverageResult {
  /** Average climb rate over the selected history window. */
  double average;

  /** Actual time span covered by the selected history window. */
  FloatDuration time_span;

  /** How the current sample was applied to the retained history. */
  ClimbSampleAction sample_action;

  /** Whether a discontinuity cleared the previous history first. */
  bool reset;

  constexpr bool IsComplete(FloatDuration required) const noexcept {
    return time_span >= required;
  }

  constexpr bool IsSampleAccepted() const noexcept {
    return sample_action != ClimbSampleAction::IGNORED;
  }
};

class ClimbAverageCalculator
{
  /**
   * Keep enough samples for a 30 second window at substantially more than
   * the normal FLARM reporting rate.  Callers supply a minimum sample
   * interval, so this is a hard memory bound rather than an assumption that
   * updates arrive at one hertz.
   */
  static constexpr int MAX_HISTORY = 256;
  struct HistoryItem
  {
    TimeStamp time;
    double altitude;

    HistoryItem() = default;

    constexpr HistoryItem(TimeStamp _time,
                          double _altitude) noexcept
      :time(_time), altitude(_altitude) {}

    bool IsDefined() const {
      return time.IsDefined();
    }

    void Reset() {
      time = TimeStamp::Undefined();
    }
  };

  HistoryItem history[MAX_HISTORY];
  int newestValIndex;
  TimeStamp last_update_time;

public:
  /**
   * Calculates the average and reports the actual history span used.
   *
   * This is deliberately separate from GetAverage() so existing consumers
   * can keep their current API while new consumers can reject short windows.
   */
  [[nodiscard]]
  ClimbAverageResult GetAverageWithSpan(TimeStamp time, double altitude,
                                        FloatDuration average_time,
                                        ClimbSamplePolicy policy={
                                          FloatDuration{0.125},
                                          FloatDuration::max(),
                                        });

  double GetAverage(TimeStamp time, double altitude,
                    FloatDuration average_time) noexcept;
  void Reset();
  bool Expired(TimeStamp now,
               FloatDuration max_age) const noexcept;
};
