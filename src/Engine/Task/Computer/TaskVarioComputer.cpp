// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskVarioComputer.hpp"
#include "Task/Stats/TaskVario.hpp"
#include "GlideSolvers/GlideResult.hpp"

TaskVarioComputer::TaskVarioComputer()
  :df(0),
   v_lpf(120, false)
{
}

void 
TaskVarioComputer::update(TaskVario &data, const GlideResult &solution)
{
  auto v = df.Update(solution.altitude_difference);
  data.value = v_lpf.Update(v);
}

void 
TaskVarioComputer::reset(TaskVario &data, const GlideResult& solution)
{
  v_lpf.Reset(0);
  df.Reset(solution.altitude_difference, 0);
  data.Reset();
}
