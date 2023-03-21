// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "TaskMacCready.hpp"

/**
 * Specialisation of TaskMacCready for task travelled
 */
class TaskMacCreadyTravelled final : public TaskMacCready
{
public:
  /**
   * Constructor for ordered task points
   *
   * @param _activeTaskPoint Current active task point in sequence
   * @param _gp Glide polar to copy for calculations
   */
  template<class I>
  TaskMacCreadyTravelled(const I tps_begin,
                         const unsigned _activeTaskPoint,
                         const GlideSettings &settings, const GlidePolar &_gp)
    :TaskMacCready(tps_begin, std::next(tps_begin, _activeTaskPoint + 1),
                   _activeTaskPoint, settings, _gp) {
  }

private:
  /* virtual methods from class TaskMacCready */
  virtual double get_min_height(const AircraftState &aircraft) const override;

  virtual GlideResult SolvePoint(const TaskPoint &tp,
                                 const AircraftState &aircraft,
                                 double minH) const override;

  virtual AircraftState get_aircraft_start(const AircraftState &aircraft) const override;
};
