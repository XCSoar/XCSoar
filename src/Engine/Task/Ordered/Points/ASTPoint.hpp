/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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


#ifndef ASTPOINT_HPP
#define ASTPOINT_HPP

#include "IntermediatePoint.hpp"

/**
 * An ASTPoint is an abstract IntermediatePoint,
 * in which the observation zone area is not used for
 * scored distance calculations (the aircraft merely has
 * to enter the observation zone)
 * but does not yet have an observation zone.
 */
class ASTPoint gcc_final : public IntermediateTaskPoint
{
public:
  /**
   * Constructor.
   * Ownership of oz is transferred to this object.  Note that AST boundaries are not scored.
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint associated with this task point
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Partially initialised object
   */
  ASTPoint(ObservationZonePoint *_oz,
           const Waypoint &wp,
           const TaskBehaviour &tb,
           bool boundary_scored=false)
    :IntermediateTaskPoint(AST, _oz, wp, tb, boundary_scored) {}
};

#endif
