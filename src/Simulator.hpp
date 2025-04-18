// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define SIMULATOR_AVAILABLE

#ifdef SIMULATOR_AVAILABLE

/**
 * Do not access this variable directly, use is_simulator() instead.
 */
extern bool global_simulator_flag;
extern bool sim_set_in_cmd_line_flag;

#endif

/**
 * Returns whether the simulator application is running
 * @return True if simulator, False if fly application
 */
static inline bool is_simulator()
{
#ifdef SIMULATOR_AVAILABLE
  return global_simulator_flag;
#else
  return false;
#endif
}
