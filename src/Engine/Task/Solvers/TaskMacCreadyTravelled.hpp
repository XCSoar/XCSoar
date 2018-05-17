/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */
#ifndef TASKMACCREADYTRAVELLED_HPP
#define TASKMACCREADYTRAVELLED_HPP

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

#endif

