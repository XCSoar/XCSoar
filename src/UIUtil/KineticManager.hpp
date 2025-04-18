// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"
#include "time/FloatDuration.hxx"

/**
 * A manager class that can be used for kinetic scrolling
 */
class KineticManager
{
  /** Time in ms until the kinetic movement is stopped */
  const FloatDuration stopping_time;

  /** Whether the kinetic movement is still active */
  bool steady = true;

  /** Position at the end of the manual movement */
  int last;

  /** Precalculated final position of the kinetic movement */
  int end;

  /** Speed at the end of the manual movement */
  double v;

  /** Clock that is used for the kinetic movement */
  PeriodClock clock;

public:
  explicit KineticManager(FloatDuration _stopping_time = std::chrono::seconds{1}) noexcept
    :stopping_time(_stopping_time) {}

  /** Needs to be called once the manual movement is started */
  void MouseDown(int x);
  /** Needs to be called on every manual mouse move event */
  void MouseMove(int x);
  /**
   * Needs to be called at the end of the manual movement and
   * starts the kinetic movement
   */
  void MouseUp(int x);

  /**
   * Returns the current position of the kinetic movement.
   * Sets the steady flag to true if the kinetic motion is
   * not active (@see IsSteady())
   */
  int GetPosition();

  /** Returns whether the kinetic movement is still active */
  bool IsSteady();
};
