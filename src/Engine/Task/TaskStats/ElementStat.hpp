/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Navigation/Geometry/GeoVector.hpp"
#include "GlideSolvers/GlideResult.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include "DistanceStat.hpp"
#include "TaskVario.hpp"
#include "Util/TypeTraits.hpp"

struct AircraftState;

/**
 * Common task element statistics.  Used because we separately want to
 * track overall task statistics as well as that of the current leg.
 */
struct ElementStat
{
  /**
   * The remaining vector.  Always check GeoVector::IsValid() before
   * using this attribute.  This is only implemented for one leg
   * (TaskStats::current_leg).
   */
  GeoVector vector_remaining;

  /** Time (s) this element was started */
  fixed time_started;
  /** Time (s) since element was started */
  fixed time_elapsed;
  /** Time (s) to element completion */
  fixed time_remaining;
  /** Time (s) of overall element */
  fixed time_planned;

  /** Gradient to element completion */
  fixed gradient;

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
   * @param ts Start time of this element (s)
   * @param state Aircraft state (to access time)
   */
  void SetTimes(const fixed ts, const AircraftState& state);

  /**
   * Determine whether the task (or subtask) is able to be finished
   * (will fail if MC too low, wind too high etc)
   *
   * @return True if can finish the task
   */
  bool IsAchievable() const {
    return solution_remaining.validity == GlideResult::RESULT_OK;
  }

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream &o, const ElementStat &es);
#endif
};

static_assert(is_trivial_clang<ElementStat>::value, "type is not trivial");

class ElementStatComputer
{
public:
  DistanceStatComputer remaining_effective;
  DistanceStatComputer remaining;
  DistanceStatComputer planned;
  DistanceStatComputer travelled;
  DistanceStatComputer pirker;
  TaskVarioComputer vario;

private:
  bool initialised;

public:
  ElementStatComputer();

  /**
   * Calculate element speeds.  Incremental speeds are
   * held at bulk speeds within first minute of elapsed time.
   *
   * @param dt Time step of sample (s)
   */
  void CalcSpeeds(ElementStat &data, const fixed dt);

  /**
   * Reset to uninitialised state, to supress calculation
   * of incremental speeds.
   */
  void Reset(ElementStat &data);
};

#endif
