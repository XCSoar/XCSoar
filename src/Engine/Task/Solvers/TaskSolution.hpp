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

#ifndef TASK_SOLUTION_HPP
#define TASK_SOLUTION_HPP

#include "Compiler.h"

struct GlideSettings;
struct GlideResult;
struct AircraftState;
class GlidePolar;
class TaskPoint;
class OrderedTaskPoint;
struct GeoPoint;
struct SpeedVector;

/**
 * Utility class for calculating glide solutions for individual points and whole tasks
 * This is used to de-couple the task system from glide calculations
 */
namespace TaskSolution
{
  /**
   * Compute optimal glide solution from aircraft to destination.
   *
   * @param taskpoint The taskpoint representing the destination
   * @param state Aircraft state at origin
   * @param polar Glide polar used for computations
   * @param minH Minimum height at destination over-ride (max of this or the task points's elevation is used)
   * @return GlideResult of task leg
   */
  gcc_pure
  GlideResult GlideSolutionRemaining(const TaskPoint& taskpoint,
                                     const AircraftState &state,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const double min_h = 0);

  gcc_pure
  GlideResult GlideSolutionRemaining(const GeoPoint &location,
                                     const GeoPoint &target,
                                     const double target_elevation,
                                     const double altitude,
                                     const SpeedVector &wind,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar);

  /**
   * Compute optimal glide solution from aircraft to destination, with
   * externally supplied sink rate.  This is used to calculate the sink
   * rate required for glide-only solutions.
   *
   * @param taskpoint The taskpoint representing the destination
   * @param state Aircraft state at origin
   * @param polar Glide polar used for computations
   * @param S Sink rate (m/s, positive down)
   * @return GlideResult of task leg
   */
  gcc_pure
  GlideResult GlideSolutionSink(const TaskPoint &taskpoint,
                                const AircraftState &state,
                                const GlideSettings &settings,
                                const GlidePolar &polar,
                                const double s);

  /**
   * Compute optimal glide solution from previous point to aircraft towards destination.
   * (For pure TaskPoints, this is null)
   *
   * @param taskpoint The taskpoint representing the destination
   * @param state Aircraft state
   * @param polar Glide polar used for computations
   * @param minH Minimum height at destination over-ride (max of this or the task points's elevation is used)
   * @return GlideResult of task leg
   */
  gcc_pure
  GlideResult GlideSolutionTravelled(const OrderedTaskPoint &taskpoint,
                                     const AircraftState &state,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const double min_h = 0);

  /**
   * Compute optimal glide solution from aircraft to destination, or modified
   * destination (e.g. where specialised TaskPoint has a target)
   *
   * @param taskpoint The taskpoint representing the destination
   * @param state Aircraft state at origin
   * @param polar Glide polar used for computations
   * @param minH Minimum height at destination over-ride (max of this or the task points's elevation is used)
   * @return GlideResult of task leg
   */
  gcc_pure
  GlideResult GlideSolutionPlanned(const OrderedTaskPoint &taskpoint,
                                   const AircraftState &state,
                                   const GlideSettings &settings,
                                   const GlidePolar &polar,
                                   const double min_h = 0);
};

#endif
