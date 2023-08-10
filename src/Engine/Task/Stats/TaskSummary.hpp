// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/TrivialArray.hxx"

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

  using TaskSummaryPointVector = TrivialArray<TaskSummaryPoint, 32u>;

  /** Vector of turnpoint data */
  TaskSummaryPointVector pts;

  constexpr void clear() noexcept {
    active = 0;
    p_remaining = 1;
    pts.clear();
  }

  void append(const TaskSummaryPoint& tsp) noexcept {
    pts.push_back(tsp);
  }

  constexpr void update(double d_remaining, double d_planned) noexcept {
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
