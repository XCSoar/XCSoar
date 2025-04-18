// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "DistanceStat.hpp"
#include "TaskVario.hpp"
#include "time/Stamp.hpp"

#include <type_traits>

/**
 * Common task element statistics.  Used because we separately want to
 * track overall task statistics as well as that of the current leg.
 */
struct ElementStat
{
  /**
   * The remaining location, i.e. the result of
   * ScoredTaskPoint::GetLocationRemaining().  Always check
   * GeoPoint::IsValid() before using this attribute.  This is only
   * implemented for one leg (TaskStats::current_leg).
   */
  GeoPoint location_remaining;

  /**
   * The remaining vector.  Always check GeoVector::IsValid() before
   * using this attribute.  This is only implemented for one leg
   * (TaskStats::current_leg).
   */
  GeoVector vector_remaining;

  /**
    * The next leg vector.  Always check GeoVector::IsValid() before
    * using this attribute.  This is only implemented for one leg
    * (TaskStats::current_leg).
    */
   GeoVector next_leg_vector;

  /** Time (s) this element was started */
  TimeStamp time_started;
  /** Time (s) since element was started */
  FloatDuration time_elapsed;

  /**
   * Time (s) remaining to element completion from now, including the
   * time to reach the start point (if task was not yet started).
   */
  FloatDuration time_remaining_now;

  /**
   * Time (s) remaining to element completion from now, excluding the
   * time to reach the start point (if task was not yet started).
   */
  FloatDuration time_remaining_start;

  /** Time (s) of overall element */
  FloatDuration time_planned;

  /** Gradient to element completion */
  double gradient;

  /** Stats for effective remaining distance of element */
  DistanceStat remaining_effective;
  /** Stats for actual remaining distance of element */
  DistanceStat remaining;
  /** Stats for overall element distance */
  DistanceStat planned;
  /** Stats for travelled distance in this element */
  DistanceStat travelled;
  /** Difference beteen planned and remaining_effective */
  DistanceStat pirker;

  /** Glide solution for planned element */
  GlideResult solution_planned;
  /** Glide solution for travelled element */
  GlideResult solution_travelled;
  /** Glide solution for remaining element */
  GlideResult solution_remaining;
  /** Glide solution for remaining element, MC=0 */
  GlideResult solution_mc0;

  /** Rate of change of altitude difference (m/s) */
  TaskVario vario;

  /** Resets all to zero. */
  void Reset() noexcept;

  /**
   * Calculate element times
   *
   * @param until_start_s the estimated time until the task start will
   * be reached [s]; zero if the task has already started
   * @param ts Start time of this element (s)
   * @param time monotonic time of day in seconds or -1 if unknown
   */
  void SetTimes(FloatDuration until_start_s,
                TimeStamp ts, TimeStamp time) noexcept;

  /**
   * Determine whether the task (or subtask) is able to be finished
   * (will fail if MC too low, wind too high etc)
   *
   * @return True if can finish the task
   */
  bool IsAchievable() const noexcept {
    return solution_remaining.IsAchievable();
  }
};

static_assert(std::is_trivial<ElementStat>::value, "type is not trivial");
