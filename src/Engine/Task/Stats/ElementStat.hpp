/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */
#ifndef ELEMENT_STAT_HPP
#define ELEMENT_STAT_HPP

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "DistanceStat.hpp"
#include "TaskVario.hpp"

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
  double time_started;
  /** Time (s) since element was started */
  double time_elapsed;

  /**
   * Time (s) remaining to element completion from now, including the
   * time to reach the start point (if task was not yet started).
   */
  double time_remaining_now;

  /**
   * Time (s) remaining to element completion, counted from the start
   * of the task.
   */
  double time_remaining_start;

  /** Time (s) of overall element */
  double time_planned;

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
  void Reset();

  /**
   * Calculate element times
   *
   * @param until_start_s the estimated time until the task start will
   * be reached [s]; zero if the task has already started
   * @param ts Start time of this element (s)
   * @param time monotonic time of day in seconds or -1 if unknown
   */
  void SetTimes(double until_start_s, double ts, double time);

  /**
   * Determine whether the task (or subtask) is able to be finished
   * (will fail if MC too low, wind too high etc)
   *
   * @return True if can finish the task
   */
  bool IsAchievable() const {
    return solution_remaining.IsAchievable();
  }
};

static_assert(std::is_trivial<ElementStat>::value, "type is not trivial");

#endif
