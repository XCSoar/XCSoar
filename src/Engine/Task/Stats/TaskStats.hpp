// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Geo/GeoBounds.hpp"
#include "ElementStat.hpp"
#include "StartStats.hpp"
#include "WindowStats.hpp"

#include <type_traits>

struct TaskBehaviour;

/** Container for common task statistics */
class TaskStats 
{
public:
  GeoBounds bounds;

  /** Total task statistics */
  ElementStat total;
  /** Current (active) leg statistics */
  ElementStat current_leg;

  /** Calculated glide angle required */
  double glide_required;
  /** Calculated cruise efficiency ratio */
  double cruise_efficiency;
  /** Calculated effective MC (m/s) */
  double effective_mc;
  /** Best MacCready setting calculated for final glide (m/s) */
  double mc_best;

  /** Nominal task distance (m) */
  double distance_nominal;
  /** Maximum task distance (m) */
  double distance_max_total;
  /** Maximum achievable task distance (m) */
  double distance_max;
  /** Minimum achievable task distance (m) */
  double distance_min;
  /** Scored distance (m) */
  double distance_scored;

  /**
   * Calculated instantaneous speed (m/s).  Negative if unknown.
   */
  double inst_speed_slow;
  double inst_speed_fast;

  /**
   * Index of the active task point.
   */
  unsigned active_index;

  /** Whether the task is navigable */
  bool task_valid;

  /** Whether ordered task has AAT areas */
  bool has_targets;

  /**
   * Is this a MAT task?
   */
  bool is_mat;

  /** Whether ordered task has optional starts */
  bool has_optional_starts;

  /** Whether the task is finished */
  bool task_finished;

  /**
   * Is the aircraft currently inside the current task point's
   * observation zone?
   */
  bool inside_oz;

  /**
   * Does the current task point need to be armed?
   */
  bool need_to_arm;

  /** Whether the task is appoximately in final glide */
  bool flight_mode_final_glide;

  StartStats start;

  WindowStats last_hour;

  constexpr FloatDuration GetEstimatedTotalTime() const noexcept {
    return total.time_elapsed + total.time_remaining_start;
  }

  /** Reset each element (for incremental speeds). */
  void reset() noexcept;

  /**
   * Convenience function, determines if remaining task is in final glide
   *
   * @return True if is mode changed
   */
  bool calc_flight_mode(const TaskBehaviour &settings) noexcept;
};

static_assert(std::is_trivial<TaskStats>::value, "type is not trivial");
