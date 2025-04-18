// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

class ClimbAverageCalculator
{
  static constexpr int MAX_HISTORY = 40;
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

public:
  double GetAverage(TimeStamp time, double altitude,
                    FloatDuration average_time) noexcept;
  void Reset();
  bool Expired(TimeStamp now,
               FloatDuration max_age) const noexcept;
};
