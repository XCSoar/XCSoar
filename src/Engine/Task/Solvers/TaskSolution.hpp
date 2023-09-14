// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  [[gnu::pure]]
  GlideResult GlideSolutionRemaining(const TaskPoint& taskpoint,
                                     const AircraftState &state,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const double min_h = 0);

  [[gnu::pure]]
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
  [[gnu::pure]]
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
  [[gnu::pure]]
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
  [[gnu::pure]]
  GlideResult GlideSolutionPlanned(const OrderedTaskPoint &taskpoint,
                                   const AircraftState &state,
                                   const GlideSettings &settings,
                                   const GlidePolar &polar,
                                   const double min_h = 0);
};
