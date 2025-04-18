// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Return type for path solver methods.
 */
enum class SolverResult {
  /**
   * Still looking for a solution.
   */
  INCOMPLETE,

  /**
   * A valid solution was found.
   */
  VALID,

  /**
   * The solver has completed, but failed to find a valid solution,
   * or the solution was not better than the previous one.  More
   * data may be required.
   */
  FAILED,
};
