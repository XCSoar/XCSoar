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

#ifndef INTERMEDIATEPOINT_H
#define INTERMEDIATEPOINT_H

#include "OrderedTaskPoint.hpp"
#include "Compiler.h"

/**
 * An IntermediatePoint is an abstract OrderedTaskPoint,
 * does not yet have an observation zone, nor defines
 * how scoring is performed within.
 * All IntermediatePoints shall have a preceding and following
 * taskpoint.
 */
class IntermediateTaskPoint: 
  public OrderedTaskPoint 
{
  double safety_height;

public:    
  /**
   * Constructor.
   *
   * @param _oz Observation zone attached to this point
   * @param wp Waypoint origin of turnpoint
   * @param tb TaskBehaviour defining options (esp safety heights)
   * @param b_scored Whether distance within OZ is scored
   *
   * @return Partially-initialised object
   */
  IntermediateTaskPoint(TaskPointType _type, ObservationZonePoint *_oz,
                        WaypointPtr &&wp,
                        const TaskBehaviour &tb,
                        const bool b_scored = false);

  gcc_pure
  bool valid() const {
    return GetPrevious() != NULL && GetNext() != NULL;
  }

  /* virtual methods from class TaskPoint */
  double GetElevation() const override;

  /* virtual methods from class OrderedTaskPoint */
  void SetTaskBehaviour(const TaskBehaviour &tb) override;
};

#endif //INTERMEDIATEPOINT_H
