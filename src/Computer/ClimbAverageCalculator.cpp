// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbAverageCalculator.hpp"

#include <cassert>

void
ClimbAverageCalculator::Reset()
{
  newestValIndex = -1;
  last_update_time = TimeStamp::Undefined();
  for (int i = 0; i < MAX_HISTORY; i++)
    history[i].Reset();
}

ClimbAverageResult
ClimbAverageCalculator::GetAverageWithSpan(TimeStamp time, double altitude,
                                           FloatDuration average_time,
                                           ClimbSamplePolicy policy)
{
  assert(average_time.count() > 0);
  assert(policy.minimum_interval.count() >= 0);
  assert(policy.maximum_gap.count() > 0);
  assert(policy.minimum_interval <= policy.maximum_gap);

  if (!time.IsDefined()) {
    Reset();
    return {0, FloatDuration::zero(), ClimbSampleAction::IGNORED, true};
  }

  bool reset = false;
  if (last_update_time.IsDefined() &&
      (time < last_update_time ||
       time > last_update_time + policy.maximum_gap)) {
    Reset();
    reset = true;
  }
  last_update_time = time;

  int bestHistory;

  const bool have_newest = newestValIndex >= 0 &&
    history[newestValIndex].IsDefined();
  const bool replace_newest = have_newest &&
    time == history[newestValIndex].time;
  const bool append_sample = !have_newest ||
    (!replace_newest &&
     time >= history[newestValIndex].time + policy.minimum_interval);

  const auto sample_action = append_sample
    ? ClimbSampleAction::APPENDED
    : replace_newest
      ? ClimbSampleAction::REPLACED
      : ClimbSampleAction::IGNORED;

  if (append_sample)
    newestValIndex = newestValIndex < MAX_HISTORY - 1 ? newestValIndex + 1 : 0;

  if (append_sample || replace_newest)
    history[newestValIndex] = HistoryItem(time, altitude);

  // initially bestHistory is the current...
  bestHistory = newestValIndex;

  // now run through the history and find the best sample
  // for average period within the average time period
  for (int i = 0; i < MAX_HISTORY; i++) {
    if (!history[i].IsDefined())
      continue;

    // outside the period -> skip value
    if (history[i].time + average_time < time)
      continue;

    // is the sample older (and therefore better) than the current found ?
    if (history[i].time < history[bestHistory].time)
      bestHistory = i;
  }

  // calculate the average !
  const auto time_span = time - history[bestHistory].time;
  const auto average = bestHistory != newestValIndex && time_span.count() > 0
    ? (altitude - history[bestHistory].altitude) / time_span.count()
    : 0;

  return {average, time_span, sample_action, reset};
}

double
ClimbAverageCalculator::GetAverage(TimeStamp time, double altitude,
                                   FloatDuration average_time) noexcept
{
  return GetAverageWithSpan(time, altitude, average_time,
                            {FloatDuration::zero(),
                             FloatDuration::max()}).average;
}

bool
ClimbAverageCalculator::Expired(TimeStamp now,
                                FloatDuration max_age) const noexcept
{
  if (!last_update_time.IsDefined())
    return true;

  return now < last_update_time || now > last_update_time + max_age;
}
