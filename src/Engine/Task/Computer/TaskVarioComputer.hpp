// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Filter.hpp"
#include "Math/DiffFilter.hpp"

class TaskVario;
struct GlideResult;

class TaskVarioComputer
{
  DiffFilter df;
  Filter v_lpf;

public:
  TaskVarioComputer();

/** 
 * Update vario, taking altitude difference from a specified glide solution
 * 
 * @param solution Solution for task element
 */
  void update(TaskVario &data, const GlideResult &solution);

/** 
 * Reset vario value (as if solution is held constant)
 * 
 * @param solution Element
 */
  void reset(TaskVario &data, const GlideResult &solution);
};
