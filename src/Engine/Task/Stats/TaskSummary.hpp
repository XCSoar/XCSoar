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
#ifndef TASK_SUMMARY_HPP
#define TASK_SUMMARY_HPP

#include "Util/TrivialArray.hxx"

#include <type_traits>

struct TaskSummaryPoint {
  /** Distance planned to this point from previous, (m) */
  double d_planned;
  /** Proportion of total distance of this point [0-1] */
  double p;
  /** Whether this task point has been achieved */
  bool achieved;
};

struct TaskSummary {
  /** Proportion of planned distance remaining, [0-1] */
  double p_remaining;
  /** Index of active taskpoint */
  unsigned active;

  typedef TrivialArray<TaskSummaryPoint, 32u> TaskSummaryPointVector;

  /** Vector of turnpoint data */
  TaskSummaryPointVector pts;

  void clear() {
    active = 0;
    p_remaining = 1;
    pts.clear();
  }
  void append(const TaskSummaryPoint& tsp) {
    pts.push_back(tsp);
  }

  void update(double d_remaining, double d_planned) {
    if (d_planned <= 0)
      return;

    p_remaining = d_remaining/d_planned;
    double p = 0;
    for (auto &i : pts) {
      p += i.d_planned / d_planned;
      i.p = p;
    }
  }
};

static_assert(std::is_trivial<TaskSummary>::value, "type is not trivial");

#endif
