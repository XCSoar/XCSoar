// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskSolveTravelled.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

#define SOLVE_ZERO

double
TaskSolveTravelled::time_error()
{
  GlideResult res = tm.glide_solution(aircraft);
  if (!res.IsOk())
    /* what can we do if there's no solution?  This is an attempt to
       make ZeroFinder ignore this call, by returning a large value.
       I'm not sure if this kludge is correct. */
    return 999999;

#ifdef SOLVE_ZERO
  auto d = res.time_elapsed - dt;
#else
  auto d = fabs(res.time_elapsed - dt);
#endif
  d += res.time_virtual;

  return ToFloatSeconds(d) * inv_dt;
}

double
TaskSolveTravelled::search(const double ce)
{
#ifdef SOLVE_ZERO
  return find_zero(ce);
#else
  return find_min(ce);
#endif
}
