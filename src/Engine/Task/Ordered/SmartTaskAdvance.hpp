// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskAdvance.hpp"

/** Class used to control advancement through an OrderedTask */
class SmartTaskAdvance final : public TaskAdvance {
  /** active advance state */
  State state = TaskAdvance::MANUAL;

public:
  State GetState() const noexcept override;

  /** 
   * Determine whether all conditions are satisfied for a turnpoint
   * to auto-advance based on condition of the turnpoint, transition
   * characteristics and advance mode.
   * 
   * @param tp The task point to check for satisfaction
   * @param state current aircraft state
   * @param x_enter whether this step transitioned enter to this tp
   * @param x_exit whether this step transitioned exit to this tp
   * 
   * @return true if this tp is ready to advance
   */
  bool CheckReadyToAdvance(const TaskPoint &tp,
                           const AircraftState &state,
                           bool x_enter, bool x_exit) noexcept override;

protected:
  void UpdateState() noexcept override;
};
