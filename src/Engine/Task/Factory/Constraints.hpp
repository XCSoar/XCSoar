/*
Copyright_License {

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

#ifndef XCSOAR_TASK_FACTORY_CONSTRAINTS_HPP
#define XCSOAR_TASK_FACTORY_CONSTRAINTS_HPP

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

#endif
