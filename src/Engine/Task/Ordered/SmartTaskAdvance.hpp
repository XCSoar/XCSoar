// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskAdvance.hpp"

/** Class used to control advancement through an OrderedTask */
class SmartTaskAdvance final : public TaskAdvance {
  /** active advance state */
  State state;

public:
  /** 
   * Constructor.  Sets defaults to auto-mode
   */
  SmartTaskAdvance();

  virtual State GetState() const;

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
  virtual bool CheckReadyToAdvance(const TaskPoint &tp,
                                   const AircraftState &state,
                                   const bool x_enter, const bool x_exit);

protected:
  virtual void UpdateState();
};
