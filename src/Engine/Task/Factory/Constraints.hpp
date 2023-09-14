// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * This struct describes the constraints imposed by the rules of an
 * #AbstractTaskFactory implementation.  Each #AbstractTaskFactory
 * implementation has one compile-time constant instance of this
 * struct.
 */
struct TaskFactoryConstraints {
  /**
   * Option to enable calculation of scores, and protect against
   * task changes if flight/task has started
   */
  bool task_scored;

  /**
   * Whether ordered task start and finish requires FAI height rules
   * and (no) speed rule.  This is the default value for
   * OrderedTaskSettings::fai_finish.
   */
  bool fai_finish;

  /**
   * Whether all turnpoints except start/finish are same type.
   */
  bool homogeneous_tps;

  /**
   * Whether start/finish turnpoints must be the same.
   */
  bool is_closed;

  /**
   * Whether start points needs to be armed.
   */
  bool start_requires_arm;

  /**
   * Minimum number of turnpoints including start/finish.
   */
  unsigned min_points;

  /**
   * Maximum number of turnpoints including start/finish.
   */
  unsigned max_points;

  /**
   * Determine if the task should have a fixed number of turnpoints
   *
   * @return True if task is fixed size
   */
  constexpr
  bool IsFixedSize() const {
    return min_points == max_points;
  }
};
